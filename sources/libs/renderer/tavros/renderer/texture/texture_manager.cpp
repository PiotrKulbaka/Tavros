#include <tavros/renderer/texture/texture_manager.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/exception.hpp>
#include <tavros/core/math/functions/basic_math.hpp>

#include <tavros/renderer/rhi/string_utils.hpp>
#include <tavros/renderer/texture/mipmap_generator.hpp>
#include <tavros/renderer/texture/texture_uploader.hpp>

namespace
{
    namespace rhi = tavros::renderer::rhi;

    tavros::core::fixed_string<512> make_key(
        tavros::core::string_view path,
        bool                      gen_mipmaps,
        rhi::texture_type         tt,
        rhi::pixel_format         pf
    )
    {
        return tavros::core::fixed_string<512>::format(
            "{}:mip={};type={};fmt={}",
            path,
            static_cast<int>(gen_mipmaps),
            rhi::to_string(tt),
            rhi::to_string(pf)
        );
    }

    tavros::core::fixed_string<1024> make_key(tavros::core::string_view path, const tavros::renderer::texture_manager::load_params& params)
    {
        return tavros::core::fixed_string<1024>::format(
            "{}:w={};h={};d={};mip={};type={};fmt={};al={};ar={};ac={}",
            path,
            params.width,
            params.height,
            params.depth,
            static_cast<int>(params.gen_mipmaps),
            rhi::to_string(params.type),
            rhi::to_string(params.pixel_format),
            params.array_layers,
            params.array_rows,
            params.array_cols
        );
    }


    rhi::pixel_format to_rhi_format(tavros::assets::image::pixel_format fmt) noexcept
    {
        switch (fmt) {
        case tavros::assets::image::pixel_format::r8:
            return rhi::pixel_format::r8un;
        case tavros::assets::image::pixel_format::rg8:
            return rhi::pixel_format::rg8un;
        case tavros::assets::image::pixel_format::rgb8:
            return rhi::pixel_format::rgb8un;
        case tavros::assets::image::pixel_format::rgba8:
            return rhi::pixel_format::rgba8un;
        default:
            TAV_UNREACHABLE();
        }
    }

    tavros::assets::image::pixel_format to_im_format(rhi::pixel_format fmt) noexcept
    {
        switch (fmt) {
        case rhi::pixel_format::r8un:
            return tavros::assets::image::pixel_format::r8;
        case rhi::pixel_format::rg8un:
            return tavros::assets::image::pixel_format::rg8;
        case rhi::pixel_format::rgb8un:
            return tavros::assets::image::pixel_format::rgb8;
        case rhi::pixel_format::rgba8un:
            return tavros::assets::image::pixel_format::rgba8;
        case rhi::pixel_format::none:
            return tavros::assets::image::pixel_format::none;
        default:
            return tavros::assets::image::pixel_format::none;
        }
    }

    static tavros::core::logger logger("texture_manager");
} // namespace

namespace tavros::renderer
{

    texture_manager::texture_manager(core::shared_ptr<assets::asset_manager> am) noexcept
        : m_am(am)
    {
        TAV_ASSERT(m_am);
    }

    void texture_manager::init(rhi::graphics_device* device) noexcept
    {
        TAV_ASSERT(device);
        TAV_ASSERT(!m_gdevice);
        m_gdevice = device;
    }

    void texture_manager::shutdown() noexcept
    {
        TAV_ASSERT(m_gdevice);
        clear();
        m_gdevice = nullptr;
    }

    texture_handle texture_manager::load(gpu_stage_buffer& stage, rhi::command_queue& cmd, core::string_view path, const load_params& params)
    {
        auto key = make_key(path, params);
        if (auto h = m_registry.find_handle(key); h.valid()) {
            // Already loaded
            acquire(h);
            return h;
        }

        // Decode image on CPU
        assets::image im;
        try {
            auto im_fmt = to_im_format(params.pixel_format);
            auto data = m_am->read_binary(path);
            im = assets::image::decode(data, im_fmt, true);
        } catch (const core::file_error& e) {
            logger.error("Failed to open image '{}'", path);
            return {};
        }

        return upload(stage, cmd, im, key, params, true);
    }

    texture_handle texture_manager::load(gpu_stage_buffer& stage, rhi::command_queue& cmd, assets::image_view im, core::string_view key, const load_params& params)
    {
        if (auto h = m_registry.find_handle(key); h.valid()) {
            // Already loaded
            acquire(h);
            return h;
        }

        return upload(stage, cmd, im, key, params, true);
    }

    void texture_manager::acquire(texture_handle handle) noexcept
    {
        m_registry.acquire(handle);
    }

    void texture_manager::release(texture_handle handle)
    {
        rhi::texture_handle gpu_handle{};
        if (const gpu_texture_view* tex_info = m_registry.find(handle)) {
            gpu_handle = tex_info->gpu_handle;
        }

        if (m_registry.release(handle)) {
            m_gdevice->safe_destroy(gpu_handle);
            m_registry.erase(handle);
        }
    }

    [[nodiscard]] const gpu_texture_view* texture_manager::get(texture_handle handle) const noexcept
    {
        return m_registry.find(handle);
    }

    rhi::texture_handle texture_manager::get_gpu_handle(texture_handle handle) const noexcept
    {
        if (const auto* tex = m_registry.find(handle)) {
            return tex->gpu_handle;
        }
        return {};
    }

    void texture_manager::clear()
    {
        TAV_ASSERT(m_gdevice);
        for (auto [h, entry] : m_registry) {
            TAV_UNUSED(h);
            auto rhi_h = entry->res.gpu_handle;
            m_gdevice->safe_destroy(rhi_h);
        }
        m_registry.clear();
    }

    texture_handle texture_manager::upload(gpu_stage_buffer& stage, rhi::command_queue& cmd, assets::image_view im, core::string_view key, const load_params& params, bool y_flip)
    {
        TAV_ASSERT(m_gdevice);

        const auto type = params.type;

        // Resolve source rect
        const uint32 src_x = params.left;
        const uint32 src_y = params.top;
        const uint32 src_w = params.width ? params.width : (im.width() > src_x ? im.width() - src_x : 0);
        const uint32 src_h = params.height ? params.height : (im.height() > src_y ? im.height() - src_y : 0);

        if (src_w == 0 || src_h == 0) {
            logger.error("Invalid source rect: {}x{} at ({}, {})", src_w, src_h, src_x, src_y);
            return {};
        }

        // Per-type resolution
        uint32 tile_w = src_w;
        uint32 tile_h = src_h;
        uint32 array_layers = params.array_layers;
        uint32 array_cols = params.array_cols;
        uint32 array_rows = params.array_rows;

        if (type == rhi::texture_type::texture_2d) {
            if (array_layers > 1) {
                if (array_cols == 0 && array_rows == 0) {
                    // Auto-detect grid
                    array_cols = static_cast<uint32>(std::ceil(std::sqrt(static_cast<double>(array_layers))));
                    array_rows = static_cast<uint32>(std::ceil(static_cast<double>(array_layers) / array_cols));
                } else if (array_cols != 0 && array_rows == 0) {
                    // cols known, derive rows
                    array_rows = math::align_up(array_layers, array_cols) / array_cols;
                } else if (array_cols == 0 && array_rows != 0) {
                    logger.error("array_rows specified without array_cols - ambiguous layout");
                    return {};
                }
                // else: both specified, use as-is

                tile_w = src_w / array_cols;
                tile_h = src_h / array_rows;

                if (tile_w == 0 || tile_h == 0) {
                    logger.error("Tile size is zero: src={}x{}, grid={}x{}", src_w, src_h, array_cols, array_rows);
                    return {};
                }
                if (tile_w * array_cols > src_w) {
                    logger.error("Grid width ({}) exceeds source width ({})", tile_w * array_cols, src_w);
                    return {};
                }
                if (tile_h * array_rows > src_h) {
                    logger.error("Grid height ({}) exceeds source height ({})", tile_h * array_rows, src_h);
                    return {};
                }
            }
        } else if (type == rhi::texture_type::texture_cube) {
            if (src_w != src_h * 6) {
                logger.error("Invalid cubemap size {}x{}, expected width == height * 6", src_w, src_h);
                return {};
            }
            tile_w = src_w / 6;
            tile_h = src_h;
            array_layers = 6;
            array_cols = 6;
            array_rows = 1;
        } else {
            logger.error("Unsupported texture type: {}", rhi::to_string(type));
            return {};
        }

        const uint32 mip_count = params.gen_mipmaps ? math::mip_levels(tile_w, tile_h) : 1u;

        // Build descriptors
        gpu_texture_view tex_info;
        tex_info.type = type;
        tex_info.format = to_rhi_format(im.format());
        tex_info.width = tile_w;
        tex_info.height = tile_h;
        tex_info.depth = 1;
        tex_info.array_layers = array_layers;

        rhi::texture_create_info rhi_tex;
        rhi_tex.type = type;
        rhi_tex.format = tex_info.format;
        rhi_tex.width = tile_w;
        rhi_tex.height = tile_h;
        rhi_tex.depth = 1;
        rhi_tex.usage = rhi::k_default_texture_usage;
        rhi_tex.mip_levels = mip_count;
        rhi_tex.array_layers = array_layers;
        rhi_tex.sample_count = 1;

        auto gpu_tex = m_gdevice->create_texture(rhi_tex);
        if (!gpu_tex) {
            logger.error("Failed to create GPU texture");
            return {};
        }

        // Upload layers
        if (array_layers == 1) {
            assets::image_view src(im, src_x, src_y, tile_w, tile_h);
            if (params.gen_mipmaps) {
                auto mips = mipmap_generator::generate(src, 0, /*srgb=*/true);
                texture_uploader::upload_2d(gpu_tex, src, mips, 0, stage, cmd);
            } else {
                texture_uploader::upload_2d_level(gpu_tex, src, 0, 0, stage, cmd);
            }
        } else {
            for (uint32 layer = 0; layer < array_layers; ++layer) {
                const uint32 col = layer % array_cols;
                const uint32 row = y_flip ? array_rows - (layer / array_cols) - 1 : (layer / array_cols);
                const uint32 offset_x = src_x + col * tile_w;
                const uint32 offset_y = src_y + row * tile_h;

                assets::image_view layer_im(im, offset_x, offset_y, tile_w, tile_h);

                // TODO: remove this (for cubemaps only)
                // Needed for proper loading of cubemaps.
                uint32 layer_index = layer;
                if (type == rhi::texture_type::texture_cube) {
                    if (layer_index == 2) {
                        layer_index = 3;
                    } else if (layer_index == 3) {
                        layer_index = 2;
                    }
                }

                if (params.gen_mipmaps) {
                    auto mips = mipmap_generator::generate(layer_im, 0, /*srgb=*/true);
                    texture_uploader::upload_2d(gpu_tex, layer_im, mips, layer_index, stage, cmd);
                } else {
                    texture_uploader::upload_2d_level(gpu_tex, layer_im, 0, layer_index, stage, cmd);
                }
            }
        }

        // Register
        tex_info.gpu_handle = gpu_tex;
        tex_info.status = core::resource_status::ready;

        auto handle = m_registry.insert(key, std::move(tex_info));
        if (!handle.valid()) {
            logger.error("Failed to insert texture '{}'", key);
            m_gdevice->safe_destroy(gpu_tex);
            return {};
        }

        logger.debug("Uploaded {}: '{}' (tile={}x{}, layers={}, grid={}x{}, mips={})", rhi::to_string(type), key, tile_w, tile_h, array_layers, array_cols, array_rows, mip_count);

        return handle;
    }

} // namespace tavros::renderer

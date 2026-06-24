#include <tavros/renderer/texture/texture_manager.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/math.hpp>
#include <tavros/core/exception.hpp>

#include <tavros/renderer/rhi/string_utils.hpp>
#include <tavros/renderer/texture/mipmap_generator.hpp>
#include <tavros/renderer/texture/texture_uploader.hpp>
#include <tavros/renderer/resource_manager.hpp>

namespace
{
    tavros::core::logger logger("texture_manager");

    tavros::renderer::rhi::pixel_format to_rhi_format(tavros::assets::image::pixel_format fmt) noexcept
    {
        using ipf = tavros::assets::image::pixel_format;
        using rpf = tavros::renderer::rhi::pixel_format;
        switch (fmt) {
        case ipf::r8:
            return rpf::r8un;
        case ipf::rg8:
            return rpf::rg8un;
        case ipf::rgb8:
            return rpf::rgb8un;
        case ipf::rgba8:
            return rpf::rgba8un;
        default:
            TAV_UNREACHABLE();
        }
    }

    tavros::assets::image::pixel_format to_im_format(tavros::renderer::rhi::pixel_format fmt) noexcept
    {
        using ipf = tavros::assets::image::pixel_format;
        using rpf = tavros::renderer::rhi::pixel_format;
        switch (fmt) {
        case rpf::r8un:
            return ipf::r8;
        case rpf::rg8un:
            return ipf::rg8;
        case rpf::rgb8un:
            return ipf::rgb8;
        case rpf::rgba8un:
            return ipf::rgba8;
        case rpf::none:
            return ipf::none;
        default:
            return ipf::none;
        }
    }
} // namespace

namespace tavros::renderer
{

    texture_manager::texture_manager(rhi::graphics_device* gdevice, assets::asset_manager* am) noexcept
        : m_gdevice(gdevice)
        , m_am(am)
    {
    }

    texture_manager::~texture_manager() noexcept
    {
        clear();
    }

    void texture_manager::release_resource(texture_t& res) noexcept
    {
        m_gdevice->safe_destroy(res.gpu_texture);
    }

    texture_manager::resource_ref_type texture_manager::load(upload_context& upctx, core::string_view name, tef::workspace& ws)
    {
        texture_desc      desc;
        core::diagnostics ds;

        tavros::tef::schema<texture_desc>::deserialize(ws.resolve_path(name), desc, ds);
        if (ds.error_count() > 0 || ds.fatal_count() > 0) {
            logger.error("Failed to load named texture '{}'", name);
            logger.flush(ds);
            return {};
        }

        if (ds.total_count() > 0) {
            logger.flush(ds);
        }

        return load(upctx, desc);
    }

    texture_manager::resource_ref_type texture_manager::load(upload_context& upctx, const texture_desc& desc)
    {
        auto key = desc.name();
        if (auto res = find(key); res) {
            // Already loaded
            acquire(res);
            return res;
        }

        // Decode image on CPU
        assets::image im;
        try {
            auto im_fmt = to_im_format(desc.load_params().pixel_format);
            auto data = m_am->read_binary(desc.load_params().path);
            im = assets::image::decode(data, im_fmt, true);
        } catch (const core::file_error& e) {
            logger.error("Failed to open image '{}'", desc.load_params().path);
            return {};
        }

        return upload(upctx, im, desc, true);
    }

    texture_manager::resource_ref_type texture_manager::load(upload_context& upctx, assets::image_view im, const texture_desc& desc)
    {
        auto key = desc.name();
        if (auto res = find(key); res) {
            // Already loaded
            acquire(res);
            return res;
        }

        return upload(upctx, im, desc, true);
    }

    texture_manager::resource_ref_type texture_manager::upload(upload_context& upctx, assets::image_view im, const texture_desc& desc, bool y_flip)
    {
        const auto& params = desc.load_params();
        auto        key = desc.name();
        const auto  type = params.type;

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
        texture_t tex(key);
        tex.type = type;
        tex.format = to_rhi_format(im.format());
        tex.width = tile_w;
        tex.height = tile_h;
        tex.depth = 1;
        tex.array_layers = array_layers;

        rhi::texture_create_info rhi_tex;
        rhi_tex.type = type;
        rhi_tex.format = tex.format;
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
                texture_uploader::upload_2d(gpu_tex, src, mips, 0, upctx);
            } else {
                texture_uploader::upload_2d_level(gpu_tex, src, 0, 0, upctx);
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
                    texture_uploader::upload_2d(gpu_tex, layer_im, mips, layer_index, upctx);
                } else {
                    texture_uploader::upload_2d_level(gpu_tex, layer_im, 0, layer_index, upctx);
                }
            }
        }

        // Register
        tex.gpu_texture = gpu_tex;

        auto ref = insert(tex);
        if (!ref) {
            logger.error("Failed to insert texture '{}'", key);
            m_gdevice->safe_destroy(gpu_tex);
            return {};
        }

        logger.debug(
            "Uploaded {}: '{}' (tile={}x{}, layers={}, grid={}x{}, mips={})",
            rhi::to_string(type),
            key,
            tile_w, tile_h,
            array_layers, array_cols, array_rows,
            mip_count
        );

        return ref;
    }

} // namespace tavros::renderer

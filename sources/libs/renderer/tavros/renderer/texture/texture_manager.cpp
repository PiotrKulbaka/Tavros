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

    tavros::core::fixed_string<512> make_key(tavros::core::string_view path, bool gen_mipmaps, rhi::texture_type tt, rhi::pixel_format pf)
    {
        return tavros::core::fixed_string<512>::format(
            "{}:mip={};type={};fmt={}",
            path,
            static_cast<int>(gen_mipmaps),
            rhi::to_string(tt),
            rhi::to_string(pf)
        );
    }

    tavros::core::fixed_string<2048> make_key(
        tavros::core::string_view p1,
        tavros::core::string_view p2,
        tavros::core::string_view p3,
        tavros::core::string_view p4,
        tavros::core::string_view p5,
        tavros::core::string_view p6,
        bool                      gen_mipmaps,
        rhi::texture_type         tt,
        rhi::pixel_format         pf
    )
    {
        return tavros::core::fixed_string<2048>::format(
            "{};{};{};{};{};{}:mip={};type={};fmt={}",
            p1, p2, p3, p4, p5, p6,
            static_cast<int>(gen_mipmaps),
            rhi::to_string(tt),
            rhi::to_string(pf)
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

    texture_handle texture_manager::load_cube(
        core::string_view path_px,
        core::string_view path_nx,
        core::string_view path_py,
        core::string_view path_ny,
        core::string_view path_pz,
        core::string_view path_nz,
        bool              gen_mipmaps,
        rhi::pixel_format pf
    )
    {
        constexpr bool k_use_srgb = true; // Use srgb conversion for mipmap generation

        const auto key = make_key(path_px, path_nx, path_py, path_ny, path_pz, path_nz, gen_mipmaps, rhi::texture_type::texture_cube, pf);

        // Already loaded
        if (auto h = m_registry.find_handle(key); h.valid()) {
            acquire(h);
            return h;
        }

        // Decode image on CPU
        core::fixed_vector<assets::image, 6> images;
        try {
            auto im_fmt = to_im_format(pf);
            auto data1 = m_am->read_binary(path_px);
            auto im1 = assets::image::decode(data1, im_fmt, /*y_flip=*/true);

            auto data2 = m_am->read_binary(path_nx);
            auto im2 = assets::image::decode(data2, im_fmt, /*y_flip=*/true);

            auto data3 = m_am->read_binary(path_ny);
            auto im3 = assets::image::decode(data3, im_fmt, /*y_flip=*/true);

            auto data4 = m_am->read_binary(path_py);
            auto im4 = assets::image::decode(data4, im_fmt, /*y_flip=*/true);

            auto data5 = m_am->read_binary(path_pz);
            auto im5 = assets::image::decode(data5, im_fmt, /*y_flip=*/true);

            auto data6 = m_am->read_binary(path_nz);
            auto im6 = assets::image::decode(data6, im_fmt, /*y_flip=*/true);

            if (!im1.valid()) {
                logger.error("Failed to decode image '{}'", path_px);
                return {};
            }

            if (!im2.valid()) {
                logger.error("Failed to decode image '{}'", path_nx);
                return {};
            }

            if (!im3.valid()) {
                logger.error("Failed to decode image '{}'", path_py);
                return {};
            }

            if (!im4.valid()) {
                logger.error("Failed to decode image '{}'", path_ny);
                return {};
            }

            if (!im5.valid()) {
                logger.error("Failed to decode image '{}'", path_pz);
                return {};
            }

            if (!im6.valid()) {
                logger.error("Failed to decode image '{}'", path_nz);
                return {};
            }

            images.push_back(std::move(im1));
            images.push_back(std::move(im2));
            images.push_back(std::move(im3));
            images.push_back(std::move(im4));
            images.push_back(std::move(im5));
            images.push_back(std::move(im6));
        } catch (const core::file_error& e) {
            logger.error("Failed to open image '{}'", e.path());
            return {};
        }

        gpu_texture_view tex_info;
        tex_info.type = rhi::texture_type::texture_cube;
        tex_info.format = pf != rhi::pixel_format::none ? pf : to_rhi_format(images[0].format());
        tex_info.width = images[0].width();
        tex_info.height = images[0].height();
        tex_info.depth = 1;

        auto handle = m_registry.insert(key, std::move(tex_info));
        if (!handle.valid()) {
            return {};
        }

        m_pending.push_back(pending_load{handle, std::move(images), gen_mipmaps, k_use_srgb, core::string(path_px)});
        return handle;
    }

    texture_handle texture_manager::load(assets::image im, core::string_view key, bool gen_mipmaps, rhi::texture_type tt, rhi::pixel_format pf)
    {
        constexpr bool k_use_srgb = true; // Use srgb conversion for mipmap generation

        // Already loaded
        if (auto h = m_registry.find_handle(key); h.valid()) {
            acquire(h);
            return h;
        }

        if (!im.valid()) {
            logger.error("Attempt to load invalid image '{}'", key);
            return {};
        }

        gpu_texture_view tex_info;
        tex_info.type = tt;
        tex_info.format = pf != rhi::pixel_format::none ? pf : to_rhi_format(im.format());
        tex_info.width = im.width();
        tex_info.height = im.height();
        tex_info.depth = 1;

        auto handle = m_registry.insert(key, std::move(tex_info));
        if (!handle.valid()) {
            return {};
        }

        m_pending.push_back(pending_load{handle, core::fixed_vector<assets::image, 6>{std::move(im)}, gen_mipmaps, k_use_srgb, core::string(key)});
        return handle;
    }

    texture_handle texture_manager::load(core::string_view path, bool gen_mipmaps, rhi::texture_type tt, rhi::pixel_format pf)
    {
        constexpr bool k_use_srgb = true; // Use srgb conversion for mipmap generation

        const auto key = make_key(path, gen_mipmaps, tt, pf);

        // Already loaded
        if (auto h = m_registry.find_handle(key); h.valid()) {
            acquire(h);
            return h;
        }

        // Decode image on CPU
        assets::image im;
        try {
            auto im_fmt = to_im_format(pf);
            auto data = m_am->read_binary(path);
            im = assets::image::decode(data, im_fmt, /*y_flip=*/true);
        } catch (const core::file_error& e) {
            logger.error("Failed to open image '{}'", e.path());
            return {};
        }

        if (!im.valid()) {
            logger.error("Failed to decode image '{}'", path);
            return {};
        }

        gpu_texture_view tex_info;
        tex_info.type = tt;
        tex_info.format = pf != rhi::pixel_format::none ? pf : to_rhi_format(im.format());
        tex_info.width = im.width();
        tex_info.height = im.height();
        tex_info.depth = 1;

        auto handle = m_registry.insert(key, std::move(tex_info));
        if (!handle.valid()) {
            return {};
        }

        m_pending.push_back(pending_load{handle, core::fixed_vector<assets::image, 6>{std::move(im)}, gen_mipmaps, k_use_srgb, core::string(path)});
        return handle;
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
            if (gpu_handle) {
                m_deferred_destroy.push_back(gpu_handle);
            }
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

    [[nodiscard]] bool texture_manager::has_pending() const noexcept
    {
        return !m_pending.empty() || !m_deferred_destroy.empty();
    }

    void texture_manager::flush_pending(rhi::graphics_device& device, gpu_stage_buffer& stage, rhi::command_queue& cmd)
    {
        // Destroy textures deferred from release()
        for (auto& h : m_deferred_destroy) {
            device.destroy_texture(h);
        }
        m_deferred_destroy.clear();

        // Upload pending textures
        for (auto& p : m_pending) {
            gpu_texture_view* tex_info = m_registry.find(p.handle);
            TAV_ASSERT(tex_info);
            if (!tex_info) {
                continue;
            }

            const uint32 mip_count = p.gen_mipmaps ? math::mip_levels(tex_info->width, tex_info->height) : 1u;

            bool is_cube = tex_info->type == rhi::texture_type::texture_cube;

            // Create GPU texture
            rhi::texture_create_info tex;
            tex.type = tex_info->type;
            tex.format = tex_info->format;
            tex.width = tex_info->width;
            tex.height = tex_info->height;
            tex.depth = tex_info->depth;
            tex.usage = rhi::k_default_texture_usage;
            tex.mip_levels = mip_count;
            tex.array_layers = is_cube ? 6 : 1;
            tex.sample_count = 1;

            auto gpu_tex = device.create_texture(tex);
            if (!gpu_tex) {
                logger.error("Failed to create GPU texture");
                tex_info->status = core::resource_status::failed;
                continue;
            }

            if (is_cube) { // cubemap
                TAV_ASSERT(p.images.size() == 6);
                for (uint32 i = 0; i < 6; ++i) {
                    if (p.gen_mipmaps) {
                        auto mips = mipmap_generator::generate(p.images[i], 0, p.use_srgb);
                        texture_uploader::upload_2d(gpu_tex, p.images[i], mips, i, stage, cmd);
                    } else {
                        texture_uploader::upload_2d_level(gpu_tex, p.images[i], 0, i, stage, cmd);
                    }
                }
            } else {
                if (p.gen_mipmaps) {
                    auto mips = mipmap_generator::generate(p.images[0], 0, p.use_srgb);
                    texture_uploader::upload_2d(gpu_tex, p.images[0], mips, 0, stage, cmd);
                } else {
                    texture_uploader::upload_2d_level(gpu_tex, p.images[0], 0, 0, stage, cmd);
                }
            }

            tex_info->gpu_handle = gpu_tex;
            tex_info->status = core::resource_status::ready;
            logger.debug("Uploaded {}: '{}' ({}x{}, {} mips)", rhi::to_string(tex_info->type), p.path, tex_info->width, tex_info->height, mip_count);
        }
        m_pending.clear();
    }

    void texture_manager::clear()
    {
        m_pending.clear();
        for (auto [h, entry] : m_registry) {
            m_deferred_destroy.push_back(entry->res.gpu_handle);
        }
        m_registry.clear();
    }

} // namespace tavros::renderer

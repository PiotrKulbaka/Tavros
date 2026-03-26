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

        m_pending.push_back(pending_load{handle, std::move(im), gen_mipmaps, k_use_srgb, core::string(key)});
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

        m_pending.push_back(pending_load{handle, std::move(im), gen_mipmaps, k_use_srgb, core::string(path)});
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

            // Create GPU texture
            rhi::texture_create_info tex;
            tex.type = tex_info->type;
            tex.format = tex_info->format;
            tex.width = tex_info->width;
            tex.height = tex_info->height;
            tex.depth = tex_info->depth;
            tex.usage = rhi::k_default_texture_usage;
            tex.mip_levels = mip_count;
            tex.array_layers = 1;
            tex.sample_count = 1;

            auto gpu_tex = device.create_texture(tex);
            if (!gpu_tex) {
                logger.error("Failed to create GPU texture");
                tex_info->status = core::resource_status::failed;
                continue;
            }

            if (p.gen_mipmaps) {
                auto mips = mipmap_generator::generate(p.im, 0, p.use_srgb);
                texture_uploader::upload_2d(gpu_tex, p.im, mips, stage, cmd);
            } else {
                texture_uploader::upload_2d_level(gpu_tex, p.im, 0, stage, cmd);
            }


            tex_info->gpu_handle = gpu_tex;
            tex_info->status = core::resource_status::ready;
            logger.debug("Texture '{}' uploaded ({}x{}, {} mips)", p.path, tex_info->width, tex_info->height, mip_count);
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

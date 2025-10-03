#include <tavros/renderer/render_system.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/rhi/texture_info.hpp>
#include <tavros/renderer/rhi/framebuffer_info.hpp>

#include <tavros/core/raii/scope_exit.hpp>

namespace
{
    tavros::core::logger logger("render_system");
}

namespace tavros::renderer
{

    render_system::render_system(rhi::graphics_device* device)
        : m_gdevice(device)
        , m_textures(&m_mallocator)
        , m_render_targets(&m_mallocator)
    {
        TAV_ASSERT(device);
    }

    render_system::~render_system()
    {
    }

    texture_view render_system::create_texture(const texture_create_info& info)
    {
        rhi::texture_info rhi_info;
        rhi_info.type = info.type;
        rhi_info.format = info.format;
        rhi_info.width = info.width;
        rhi_info.height = info.height;
        rhi_info.depth = info.depth;
        rhi_info.usage = rhi::k_default_texture_usage;
        rhi_info.mip_levels = info.mip_levels;
        rhi_info.array_layers = info.array_layers;
        rhi_info.sample_count = 1;

        auto rhi_tex_h = m_gdevice->create_texture(rhi_info);
        if (rhi_tex_h == rhi::texture_handle::invalid()) {
            return texture_view();
        }

        auto tex_h = m_textures.emplace_add(rhi_tex_h, info);
        return texture_view(&m_textures, tex_h);
    }

    void render_system::release_texture(texture_view tex)
    {
        auto tex_h = tex.handle();
        if (!m_textures.exists(tex_h)) {
            ::logger.error("Failed to release texture {}: not exists", tex_h);
            return;
        }

        auto rhi_tex_h = tex->handle();
        m_textures.erase(tex_h);
        m_gdevice->destroy_texture(rhi_tex_h);
    }

    render_target_view render_system::create_render_target(const render_target_create_info& info)
    {
        using color_attachments_t = core::static_vector<rhi::texture_handle, rhi::k_max_color_attachments>;
        color_attachments_t color_attachments;

        auto color_attachments_guard = core::make_scope_exit([&]() {
            for (auto i = 0; i < color_attachments.size(); ++i) {
                m_gdevice->destroy_texture(color_attachments[i]);
            }
        });

        for (auto i = 0; i < info.color_attachment_formats.size(); ++i) {
            rhi::texture_info rhi_color_tex_info;
            rhi_color_tex_info.type = info.target_type;
            rhi_color_tex_info.format = info.color_attachment_formats[i];
            rhi_color_tex_info.width = info.width;
            rhi_color_tex_info.height = info.height;
            rhi_color_tex_info.depth = info.depth;
            rhi_color_tex_info.usage = rhi::texture_usage::render_target;
            rhi_color_tex_info.mip_levels = 1;
            rhi_color_tex_info.array_layers = 1;
            rhi_color_tex_info.sample_count = info.sample_count;

            if (info.sample_count > 1) {
                rhi_color_tex_info.usage |= rhi::texture_usage::resolve_source;
            } else {
                rhi_color_tex_info.usage |= rhi::texture_usage::sampled;
            }

            auto color_attachment_h = m_gdevice->create_texture(rhi_color_tex_info);
            if (color_attachment_h == rhi::texture_handle::invalid()) {
                return render_target_view();
            }
            color_attachments.push_back(color_attachment_h);
        }

        rhi::texture_handle depth_stencil_attachment = rhi::texture_handle::invalid();
        auto                depth_stencil_attachment_guard = core::make_scope_exit([&]() {
            if (depth_stencil_attachment != rhi::texture_handle::invalid()) {
                m_gdevice->destroy_texture(depth_stencil_attachment);
            }
        });

        if (info.depth_stencil_attachment_format != rhi::pixel_format::none) {
            rhi::texture_info rhi_depth_stencil_tex_info;
            rhi_depth_stencil_tex_info.type = info.target_type;
            rhi_depth_stencil_tex_info.format = info.depth_stencil_attachment_format;
            rhi_depth_stencil_tex_info.width = info.width;
            rhi_depth_stencil_tex_info.height = info.height;
            rhi_depth_stencil_tex_info.depth = info.depth;
            rhi_depth_stencil_tex_info.usage = rhi::texture_usage::depth_stencil_target;
            rhi_depth_stencil_tex_info.mip_levels = 1;
            rhi_depth_stencil_tex_info.array_layers = 1;
            rhi_depth_stencil_tex_info.sample_count = info.sample_count;

            if (info.sample_count > 1) {
                rhi_depth_stencil_tex_info.usage |= rhi::texture_usage::resolve_source;
            }

            auto rhi_depth_stencil_attachment_h = m_gdevice->create_texture(rhi_depth_stencil_tex_info);
            if (rhi_depth_stencil_attachment_h == rhi::texture_handle::invalid()) {
                return render_target_view();
            }
            depth_stencil_attachment = rhi_depth_stencil_attachment_h;
        }

        rhi::framebuffer_info rhi_framebuffer_info;
        rhi_framebuffer_info.width = info.width;
        rhi_framebuffer_info.height = info.height;
        rhi_framebuffer_info.color_attachment_formats = info.color_attachment_formats;
        rhi_framebuffer_info.depth_stencil_attachment_format = info.depth_stencil_attachment_format;
        rhi_framebuffer_info.sample_count = info.sample_count;

        core::optional<rhi::texture_handle> optional_depth_stencil;
        if (depth_stencil_attachment != rhi::texture_handle::invalid()) {
            optional_depth_stencil = depth_stencil_attachment;
        }
        auto rhi_framebuffer_h = m_gdevice->create_framebuffer(rhi_framebuffer_info, color_attachments, optional_depth_stencil);
        if (rhi_framebuffer_h == rhi::framebuffer_handle::invalid()) {
            return render_target_view();
        }

        color_attachments_guard.release();
        depth_stencil_attachment_guard.release();

        auto framebuffer_h = m_render_targets.emplace_add(rhi_framebuffer_h, info, color_attachments, depth_stencil_attachment);
        return render_target_view(&m_render_targets, framebuffer_h);
    }

    void render_system::release_render_target(render_target_view rt)
    {
        auto rt_h = rt.handle();
        if (!m_render_targets.exists(rt_h)) {
            ::logger.error("Failed to release render_target {}: not exists", rt_h);
            return;
        }

        auto rhi_fb_h = rt->handle();
        m_gdevice->destroy_framebuffer(rhi_fb_h);

        auto count = rt->color_attachment_count();
        for (uint32 i = 0; i < count; ++i) {
            m_gdevice->destroy_texture(rt->color_attachment(i));
        }

        if (rt->depth_stencil_attachment() != rhi::texture_handle::invalid()) {
            m_gdevice->destroy_texture(rt->depth_stencil_attachment());
        }

        m_render_targets.erase(rt_h);
    }

} // namespace tavros::renderer

#include <tavros/renderer/render_target.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/raii/scope_exit.hpp>
#include <tavros/core/debug/assert.hpp>


namespace
{
    tavros::core::logger logger("render_target");
}

namespace tavros::renderer
{

    render_target::render_target(tavros::core::buffer_view<rhi::pixel_format> cl_atch_fmts, rhi::pixel_format ds_atch_fmt) noexcept
        : m_is_created(false)
        , m_gdevice(nullptr)
        , m_color_attachment_formats(cl_atch_fmts)
        , m_depth_stencil_attachment_format(ds_atch_fmt)
    {
        for (auto fmt : m_color_attachment_formats) {
            TAV_ASSERT(fmt != rhi::pixel_format::none);
        }
    }

    render_target::~render_target()
    {
        shutdown();
    }

    void render_target::init(rhi::graphics_device* gdevice)
    {
        if (m_gdevice) {
            ::logger.warning("Already initialized");
        }
        m_gdevice = gdevice;
        TAV_ASSERT(m_gdevice);
    }

    void render_target::shutdown()
    {
        if (m_is_created) {
            destroy_all();
        }
    }

    void render_target::recreate(uint32 width, uint32 height, uint32 msaa)
    {
        destroy_all();

        auto exit_fail = core::make_scope_exit([this]() { destroy_all(); });

        constexpr auto resolve_source_usage = rhi::texture_usage::resolve_source | rhi::texture_usage::render_target;
        constexpr auto resolve_destination_usage = rhi::texture_usage::resolve_destination | rhi::texture_usage::sampled | rhi::texture_usage::transfer_source;

        for (auto fmt : m_color_attachment_formats) {
            auto src_tex = create_texture(width, height, fmt, resolve_source_usage, msaa);
            if (!src_tex) {
                return;
            }

            m_resolve_source_color_attachments.push_back(src_tex);

            auto dst_tex = create_texture(width, height, fmt, resolve_destination_usage, 1);
            if (!dst_tex) {
                return;
            }

            m_resolve_destination_color_attachments.push_back(dst_tex);
        }

        m_resolve_source_depth_stencil_attachment = create_texture(width, height, m_depth_stencil_attachment_format, resolve_source_usage, msaa);
        if (!m_resolve_source_depth_stencil_attachment) {
            return;
        }

        m_resolve_destination_depth_stencil_attachment = create_texture(width, height, m_depth_stencil_attachment_format, resolve_destination_usage, 1);
        if (!m_resolve_destination_depth_stencil_attachment) {
            return;
        }

        m_framebuffer = create_fb(width, height, msaa);
        if (!m_framebuffer) {
            return;
        }

        m_render_pass = create_rp(true);
        if (!m_render_pass) {
            return;
        }

        exit_fail.release();

        m_is_created = true;
    }

    uint32 render_target::color_attachment_count() const
    {
        return m_color_attachment_formats.size();
    }

    rhi::texture_handle render_target::get_color_attachment(uint32 index) const
    {
        return m_resolve_destination_color_attachments[index];
    }

    rhi::texture_handle render_target::get_depth_stencil_attachment() const
    {
        return m_resolve_destination_depth_stencil_attachment;
    }

    rhi::framebuffer_handle render_target::framebuffer() const
    {
        TAV_ASSERT(m_framebuffer);
        return m_framebuffer;
    }

    rhi::render_pass_handle render_target::render_pass() const
    {
        return m_render_pass;
    }

    void render_target::destroy_all()
    {
        m_is_created = false;

        for (auto tex : m_resolve_source_color_attachments) {
            m_gdevice->destroy_texture(tex);
        }
        m_resolve_source_color_attachments.clear();

        for (auto tex : m_resolve_destination_color_attachments) {
            m_gdevice->destroy_texture(tex);
        }
        m_resolve_destination_color_attachments.clear();

        if (m_resolve_source_depth_stencil_attachment) {
            m_gdevice->destroy_texture(m_resolve_source_depth_stencil_attachment);
            m_resolve_source_depth_stencil_attachment = {};
        }

        if (m_resolve_destination_depth_stencil_attachment) {
            m_gdevice->destroy_texture(m_resolve_destination_depth_stencil_attachment);
            m_resolve_destination_depth_stencil_attachment = {};
        }

        if (m_framebuffer) {
            m_gdevice->destroy_framebuffer(m_framebuffer);
            m_framebuffer = {};
        }

        if (m_render_pass) {
            m_gdevice->destroy_render_pass(m_render_pass);
            m_render_pass = {};
        }
    }

    rhi::framebuffer_handle render_target::create_fb(uint32 width, uint32 height, uint32 msaa)
    {
        rhi::framebuffer_create_info fb_info;
        fb_info.width = width;
        fb_info.height = height;
        fb_info.color_attachments = m_resolve_source_color_attachments;
        fb_info.has_depth_stencil_attachment = true;
        fb_info.depth_stencil_attachment = m_resolve_source_depth_stencil_attachment;
        fb_info.sample_count = msaa;

        auto fb = m_gdevice->create_framebuffer(fb_info);
        if (!fb) {
            ::logger.fatal("Failed to create framebuffer.");
            return {};
        }
        return fb;
    }

    rhi::texture_handle render_target::create_texture(uint32 width, uint32 height, rhi::pixel_format fmt, tavros::core::flags<rhi::texture_usage> usage, uint32 msaa)
    {
        rhi::texture_create_info tex_info;
        tex_info.type = rhi::texture_type::texture_2d;
        tex_info.format = fmt;
        tex_info.width = width;
        tex_info.height = height;
        tex_info.depth = 1;
        tex_info.usage = usage;
        tex_info.mip_levels = 1;
        tex_info.array_layers = 1;
        tex_info.sample_count = msaa;

        auto tex = m_gdevice->create_texture(tex_info);
        if (!tex) {
            ::logger.fatal("Failed to create texture.");
            return {};
        }
        return tex;
    }

    rhi::render_pass_handle render_target::create_rp(bool need_resolve)
    {
        rhi::render_pass_create_info rp_info;

        uint32 resolve_index = 0;
        for (uint32 index = 0; index < m_color_attachment_formats.size(); ++index) {
            rhi::color_attachment_info ca_info;
            ca_info.format = m_color_attachment_formats[index];
            ca_info.load = rhi::load_op::clear;
            ca_info.store = need_resolve ? rhi::store_op::resolve : rhi::store_op::store;
            ca_info.resolve_target = need_resolve ? m_resolve_destination_color_attachments[index] : rhi::texture_handle();
            ca_info.clear_value[0] = 0.2f;
            ca_info.clear_value[1] = 0.2f;
            ca_info.clear_value[2] = 0.25f;
            ca_info.clear_value[3] = 1.0f;

            rp_info.color_attachments.push_back(ca_info);
        }

        rhi::depth_stencil_attachment_info dsca_info;
        dsca_info.format = m_depth_stencil_attachment_format;
        dsca_info.depth_load = rhi::load_op::clear;
        dsca_info.depth_store = need_resolve ? rhi::store_op::resolve : rhi::store_op::store;
        dsca_info.depth_clear_value = 1.0f;
        dsca_info.stencil_load = rhi::load_op::dont_care;
        dsca_info.stencil_store = rhi::store_op::dont_care;
        dsca_info.stencil_clear_value = 0;
        dsca_info.resolve_target = m_resolve_destination_depth_stencil_attachment;

        rp_info.depth_stencil_attachment = dsca_info;

        auto rp = m_gdevice->create_render_pass(rp_info);
        if (!rp) {
            ::logger.fatal("Failed to create render pass.");
            return {};
        }

        return rp;
    }

} // namespace tavros::renderer

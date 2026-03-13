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

    render_target::render_target() noexcept
        : m_is_init(false)
        , m_is_created(false)
        , m_is_msaa_enabled(false)
        , m_gdevice(nullptr)
        , m_ds_fmt(rhi::pixel_format::none)
    {
    }

    render_target::render_target(render_target&& other) noexcept
        : m_is_init(other.m_is_init)
        , m_is_created(other.m_is_created)
        , m_is_msaa_enabled(other.m_is_msaa_enabled)
        , m_gdevice(other.m_gdevice)
        , m_cl_fmt(std::move(other.m_cl_fmt))
        , m_ds_fmt(other.m_ds_fmt)
        , m_src_cl(std::move(other.m_src_cl))
        , m_dst_cl(std::move(other.m_dst_cl))
        , m_src_ds(other.m_src_ds)
        , m_dst_ds(other.m_dst_ds)
        , m_framebuffer(other.m_framebuffer)
        , m_render_pass(other.m_render_pass)
    {
        other.m_is_init = false;
        other.m_is_created = false;
        other.m_is_msaa_enabled = false;
        other.m_gdevice = nullptr;
        other.m_ds_fmt = rhi::pixel_format::none;
        other.m_src_ds = {};
        other.m_dst_ds = {};
        other.m_framebuffer = {};
        other.m_render_pass = {};
    }

    render_target::~render_target() noexcept
    {
        shutdown();
    }

    render_target& render_target::operator=(render_target&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        shutdown();

        m_is_init = other.m_is_init;
        m_is_created = other.m_is_created;
        m_is_msaa_enabled = other.m_is_msaa_enabled;
        m_gdevice = other.m_gdevice;
        m_cl_fmt = std::move(other.m_cl_fmt);
        m_ds_fmt = other.m_ds_fmt;
        m_src_cl = std::move(other.m_src_cl);
        m_dst_cl = std::move(other.m_dst_cl);
        m_src_ds = other.m_src_ds;
        m_dst_ds = other.m_dst_ds;
        m_framebuffer = other.m_framebuffer;
        m_render_pass = other.m_render_pass;

        other.m_is_init = false;
        other.m_is_created = false;
        other.m_is_msaa_enabled = false;
        other.m_gdevice = nullptr;
        other.m_ds_fmt = rhi::pixel_format::none;
        other.m_src_ds = {};
        other.m_dst_ds = {};
        other.m_framebuffer = {};
        other.m_render_pass = {};

        return *this;
    }

    void render_target::init(rhi::graphics_device* gdevice, tavros::core::buffer_view<rhi::pixel_format> cl_atch_fmts, rhi::pixel_format ds_atch_fmt)
    {
        if (m_is_init) {
            ::logger.error("Already initialized");
            return;
        }
        TAV_ASSERT(!m_gdevice);
        TAV_ASSERT(gdevice);

        for (auto fmt : cl_atch_fmts) {
            TAV_ASSERT(fmt != rhi::pixel_format::none);
            m_cl_fmt.push_back(fmt);
        }

        m_ds_fmt = ds_atch_fmt;
        m_gdevice = gdevice;
        m_is_init = true;
    }

    void render_target::shutdown()
    {
        if (m_is_created) {
            destroy_all();
        }
        m_is_init = false;
        m_ds_fmt = rhi::pixel_format::none;
        m_cl_fmt.clear();
    }

    void render_target::resize(uint32 width, uint32 height, uint32 msaa)
    {
        TAV_ASSERT(m_is_init);
        TAV_ASSERT(m_gdevice);

        destroy_all();

        const bool need_resolve = msaa > 1;

        auto exit_fail = core::make_scope_exit([this]() { destroy_all(); });

        constexpr auto color_usage_msaa = rhi::texture_usage::resolve_source | rhi::texture_usage::render_target;
        constexpr auto color_usage_resolved = rhi::texture_usage::resolve_destination | rhi::texture_usage::sampled | rhi::texture_usage::transfer_source;
        constexpr auto color_usage_simple = rhi::texture_usage::render_target | rhi::texture_usage::sampled | rhi::texture_usage::transfer_source;

        constexpr auto depth_usage_msaa = rhi::texture_usage::resolve_source | rhi::texture_usage::render_target;
        constexpr auto depth_usage_resolved = rhi::texture_usage::resolve_destination | rhi::texture_usage::sampled | rhi::texture_usage::transfer_source;
        constexpr auto depth_usage_simple = rhi::texture_usage::render_target | rhi::texture_usage::sampled | rhi::texture_usage::transfer_source;

        for (auto fmt : m_cl_fmt) {
            if (need_resolve) {
                auto src_tex = create_texture(width, height, fmt, color_usage_msaa, msaa);
                if (!src_tex) {
                    return;
                }
                m_src_cl.push_back(src_tex);

                auto dst_tex = create_texture(width, height, fmt, color_usage_resolved, 1);
                if (!dst_tex) {
                    return;
                }
                m_dst_cl.push_back(dst_tex);
            } else {
                auto tex = create_texture(width, height, fmt, color_usage_simple, 1);
                if (!tex) {
                    return;
                }
                m_src_cl.push_back(tex);
                m_dst_cl.push_back(tex);
            }
        }

        if (need_resolve) {
            m_src_ds = create_texture(width, height, m_ds_fmt, depth_usage_msaa, msaa);
            if (!m_src_ds) {
                return;
            }

            m_dst_ds = create_texture(width, height, m_ds_fmt, depth_usage_resolved, 1);
            if (!m_dst_ds) {
                return;
            }
        } else {
            auto tex = create_texture(width, height, m_ds_fmt, depth_usage_simple, 1);
            if (!tex) {
                return;
            }
            m_src_ds = tex;
            m_dst_ds = tex;
        }

        m_framebuffer = create_fb(width, height, msaa);
        if (!m_framebuffer) {
            return;
        }

        m_render_pass = create_rp(need_resolve);
        if (!m_render_pass) {
            return;
        }

        exit_fail.release();


        m_is_msaa_enabled = msaa > 1;
        m_is_created = true;
    }

    uint32 render_target::attachment_count() const
    {
        return m_cl_fmt.size();
    }

    rhi::texture_handle render_target::color_attachment(uint32 index) const
    {
        TAV_ASSERT(m_is_init);
        TAV_ASSERT(m_is_created);
        TAV_ASSERT(index < attachment_count());
        return m_dst_cl[index];
    }

    rhi::texture_handle render_target::depth_stencil_attachment() const
    {
        TAV_ASSERT(m_is_init);
        TAV_ASSERT(m_is_created);
        return m_dst_ds;
    }

    rhi::framebuffer_handle render_target::framebuffer() const
    {
        TAV_ASSERT(m_is_init);
        TAV_ASSERT(m_framebuffer);
        return m_framebuffer;
    }

    rhi::render_pass_handle render_target::render_pass() const
    {
        TAV_ASSERT(m_is_init);
        TAV_ASSERT(m_render_pass);
        return m_render_pass;
    }

    void render_target::destroy_all()
    {
        if (!m_gdevice) {
            return;
        }

        m_is_created = false;

        for (auto tex : m_src_cl) {
            m_gdevice->destroy_texture(tex);
        }
        m_src_cl.clear();

        if (m_is_msaa_enabled) {
            for (auto tex : m_dst_cl) {
                m_gdevice->destroy_texture(tex);
            }
        }
        m_dst_cl.clear();

        if (m_src_ds) {
            m_gdevice->destroy_texture(m_src_ds);
            m_src_ds = {};
        }

        if (m_dst_ds && m_is_msaa_enabled) {
            m_gdevice->destroy_texture(m_dst_ds);
        }
        m_dst_ds = {};

        if (m_framebuffer) {
            m_gdevice->destroy_framebuffer(m_framebuffer);
            m_framebuffer = {};
        }

        if (m_render_pass) {
            m_gdevice->destroy_render_pass(m_render_pass);
            m_render_pass = {};
        }

        m_is_msaa_enabled = false;
    }

    rhi::framebuffer_handle render_target::create_fb(uint32 width, uint32 height, uint32 msaa)
    {
        TAV_ASSERT(m_is_init);

        rhi::framebuffer_create_info fb_info;
        fb_info.width = width;
        fb_info.height = height;
        fb_info.color_attachments = m_src_cl;
        fb_info.has_depth_stencil_attachment = m_ds_fmt != rhi::pixel_format::none;
        fb_info.depth_stencil_attachment = m_src_ds;
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
        TAV_ASSERT(m_is_init);

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
        TAV_ASSERT(m_is_init);

        rhi::render_pass_create_info rp_info;

        uint32 resolve_index = 0;
        for (uint32 index = 0; index < m_cl_fmt.size(); ++index) {
            rhi::color_attachment_info ca_info;
            ca_info.format = m_cl_fmt[index];
            ca_info.load = rhi::load_op::clear;
            ca_info.store = need_resolve ? rhi::store_op::resolve : rhi::store_op::store;
            ca_info.resolve_target = need_resolve ? m_dst_cl[index] : rhi::texture_handle();
            ca_info.clear_value[0] = 0.0f;
            ca_info.clear_value[1] = 0.0f;
            ca_info.clear_value[2] = 0.0f;
            ca_info.clear_value[3] = 0.0f;

            rp_info.color_attachments.push_back(ca_info);
        }

        rhi::depth_stencil_attachment_info dsca_info;
        dsca_info.format = m_ds_fmt;
        dsca_info.depth_load = rhi::load_op::clear;
        dsca_info.depth_store = need_resolve ? rhi::store_op::resolve : rhi::store_op::store;
        dsca_info.depth_clear_value = 1.0f;
        dsca_info.stencil_load = rhi::load_op::dont_care;
        dsca_info.stencil_store = rhi::store_op::dont_care;
        dsca_info.stencil_clear_value = 0;
        dsca_info.resolve_target = need_resolve ? m_dst_ds : rhi::texture_handle();

        rp_info.depth_stencil_attachment = dsca_info;

        auto rp = m_gdevice->create_render_pass(rp_info);
        if (!rp) {
            ::logger.fatal("Failed to create render pass.");
            return {};
        }

        return rp;
    }

} // namespace tavros::renderer

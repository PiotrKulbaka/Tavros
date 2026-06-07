#include <tavros/renderer/render_target/render_target.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/raii/scope_exit.hpp>
#include <tavros/core/debug/assert.hpp>


namespace
{
    tavros::core::logger logger("render_target");

    using pf = tavros::renderer::rhi::pixel_format;

    pf combine_depth_stencil_formats(pf df, pf sf) noexcept
    {
        TAV_ASSERT(df == pf::depth24 || df == pf::depth32f || df == pf::none);
        TAV_ASSERT(sf == pf::stencil8 || sf == pf::none);

        if (df == pf::depth24) {
            if (sf == pf::stencil8) {
                return pf::depth24_stencil8;
            } else if (sf == pf::none) {
                return pf::depth24;
            }
        } else if (df == pf::depth32f) {
            if (sf == pf::stencil8) {
                return pf::depth32f_stencil8;
            } else if (sf == pf::none) {
                return pf::depth32f;
            }
        } else if (df == pf::none) {
            if (sf == pf::stencil8) {
                return pf::stencil8;
            } else if (sf == pf::none) {
                return pf::none;
            }
        }
        TAV_ASSERT(false);
        return pf::none;
    }
} // namespace

namespace tavros::renderer
{

    render_target::render_target(rhi::graphics_device* gdevice, const render_target_desc& desc) noexcept
        : m_required_msaa(desc.multisample().sample_count)
        , m_current_msaa(0)
        , m_gdevice(gdevice)
        , m_da_cfg(desc.depth_attachment())
        , m_sa_cfg(desc.stencil_attachment())
    {
        logger.info("{} created", fmt::styled_name(desc.name()));

        TAV_ASSERT(m_gdevice);

        for (auto& c : desc.color_attachments()) {
            TAV_ASSERT(c.format != rhi::pixel_format::none);
            m_ca_cfg.push_back(c);
        }
    }

    render_target::render_target(render_target&& other) noexcept
        : m_required_msaa(other.m_required_msaa)
        , m_current_msaa(other.m_current_msaa)
        , m_gdevice(other.m_gdevice)
        , m_ca_cfg(std::move(other.m_ca_cfg))
        , m_da_cfg(std::move(other.m_da_cfg))
        , m_sa_cfg(std::move(other.m_sa_cfg))
        , m_src_cl(std::move(other.m_src_cl))
        , m_dst_cl(std::move(other.m_dst_cl))
        , m_src_ds(other.m_src_ds)
        , m_dst_ds(other.m_dst_ds)
        , m_framebuffer(other.m_framebuffer)
        , m_render_pass(other.m_render_pass)
    {
        other.m_required_msaa = 0;
        other.m_current_msaa = 0;
        other.m_gdevice = nullptr;
        other.m_ca_cfg.clear();
        other.m_da_cfg = {};
        other.m_sa_cfg = {};
        other.m_src_cl.clear();
        other.m_dst_cl.clear();
        other.m_src_ds = {};
        other.m_dst_ds = {};
        other.m_framebuffer = {};
        other.m_render_pass = {};
    }

    render_target::~render_target() noexcept
    {
        destroy_all();
    }

    render_target& render_target::operator=(render_target&& other) noexcept
    {
        if (this == &other) {
            return *this;
        }
        destroy_all();

        m_required_msaa = other.m_required_msaa;
        m_current_msaa = other.m_current_msaa;
        m_gdevice = other.m_gdevice;
        m_ca_cfg = std::move(other.m_ca_cfg);
        m_da_cfg = std::move(other.m_da_cfg);
        m_sa_cfg = std::move(other.m_sa_cfg);
        m_src_cl = std::move(other.m_src_cl);
        m_dst_cl = std::move(other.m_dst_cl);
        m_src_ds = other.m_src_ds;
        m_dst_ds = other.m_dst_ds;
        m_framebuffer = other.m_framebuffer;
        m_render_pass = other.m_render_pass;

        other.m_required_msaa = 0;
        other.m_current_msaa = 0;
        other.m_gdevice = nullptr;
        other.m_ca_cfg.clear();
        other.m_da_cfg = {};
        other.m_sa_cfg = {};
        other.m_src_cl.clear();
        other.m_dst_cl.clear();
        other.m_src_ds = {};
        other.m_dst_ds = {};
        other.m_framebuffer = {};
        other.m_render_pass = {};

        return *this;
    }

    void render_target::resize(uint32 width, uint32 height, uint32 max_msaa)
    {
        TAV_ASSERT(m_gdevice);

        destroy_all();
        auto exit_fail = core::make_scope_exit([this]() { destroy_all(); });

        m_current_msaa = m_required_msaa > max_msaa ? max_msaa : m_required_msaa;

        constexpr auto color_usage_msaa = rhi::texture_usage::resolve_source | rhi::texture_usage::render_target;
        constexpr auto color_usage_resolved = rhi::texture_usage::resolve_destination | rhi::texture_usage::sampled | rhi::texture_usage::transfer_source;
        constexpr auto color_usage_simple = rhi::texture_usage::render_target | rhi::texture_usage::sampled | rhi::texture_usage::transfer_source;

        constexpr auto depth_usage_msaa = rhi::texture_usage::resolve_source | rhi::texture_usage::render_target;
        constexpr auto depth_usage_resolved = rhi::texture_usage::resolve_destination | rhi::texture_usage::sampled | rhi::texture_usage::transfer_source;
        constexpr auto depth_usage_simple = rhi::texture_usage::render_target | rhi::texture_usage::sampled | rhi::texture_usage::transfer_source;

        for (const auto& c : m_ca_cfg) {
            if (m_current_msaa > 1) {
                auto src_tex = create_texture(width, height, c.format, color_usage_msaa, m_current_msaa);
                if (!src_tex) {
                    return;
                }
                m_src_cl.push_back(src_tex);

                auto dst_tex = create_texture(width, height, c.format, color_usage_resolved, 1);
                if (!dst_tex) {
                    return;
                }
                m_dst_cl.push_back(dst_tex);
            } else {
                auto tex = create_texture(width, height, c.format, color_usage_simple, 1);
                if (!tex) {
                    return;
                }
                m_src_cl.push_back(tex);
                m_dst_cl.push_back(tex);
            }
        }

        auto ds_fmt = combine_depth_stencil_formats(m_da_cfg.format, m_sa_cfg.format);
        if (ds_fmt != rhi::pixel_format::none) {
            if (m_current_msaa > 1) {
                m_src_ds = create_texture(width, height, ds_fmt, depth_usage_msaa, m_current_msaa);
                if (!m_src_ds) {
                    return;
                }

                m_dst_ds = create_texture(width, height, ds_fmt, depth_usage_resolved, 1);
                if (!m_dst_ds) {
                    return;
                }
            } else {
                auto tex = create_texture(width, height, ds_fmt, depth_usage_simple, 1);
                if (!tex) {
                    return;
                }
                m_src_ds = tex;
                m_dst_ds = tex;
            }
        }

        m_framebuffer = create_fb(width, height);
        if (!m_framebuffer) {
            return;
        }

        m_render_pass = create_rp();
        if (!m_render_pass) {
            return;
        }

        exit_fail.release();
    }

    uint32 render_target::attachment_count() const
    {
        return m_ca_cfg.size();
    }

    rhi::texture_handle render_target::color_attachment_at(uint32 index) const noexcept
    {
        TAV_ASSERT(index < attachment_count());
        return m_dst_cl[index];
    }

    rhi::texture_handle render_target::color_attachment_by_name(core::string_view name) const noexcept
    {
        uint32 index = 0;
        for (const auto& cur : m_ca_cfg) {
            if (cur.name == name) {
                return m_dst_cl[index];
            }
            ++index;
        }
        return {};
    }

    rhi::texture_handle render_target::depth_stencil_attachment() const
    {
        return m_dst_ds;
    }

    rhi::framebuffer_handle render_target::framebuffer() const
    {
        return m_framebuffer;
    }

    rhi::render_pass_handle render_target::render_pass() const
    {
        return m_render_pass;
    }

    void render_target::destroy_all()
    {
        if (!m_gdevice) {
            return;
        }

        for (auto tex : m_src_cl) {
            m_gdevice->destroy_texture(tex);
        }
        m_src_cl.clear();

        if (m_current_msaa > 1) {
            for (auto tex : m_dst_cl) {
                m_gdevice->destroy_texture(tex);
            }
        }
        m_dst_cl.clear();

        if (m_src_ds) {
            m_gdevice->destroy_texture(m_src_ds);
            m_src_ds = {};
        }

        if (m_current_msaa > 1) {
            if (m_dst_ds) {
                m_gdevice->destroy_texture(m_dst_ds);
            }
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

        m_current_msaa = 0;
    }

    rhi::framebuffer_handle render_target::create_fb(uint32 width, uint32 height)
    {
        rhi::framebuffer_create_info fb_info;
        fb_info.width = width;
        fb_info.height = height;
        fb_info.color_attachments = m_src_cl;
        fb_info.has_depth_stencil_attachment = m_da_cfg.format != rhi::pixel_format::none || m_sa_cfg.format != rhi::pixel_format::none;
        fb_info.depth_stencil_attachment = m_src_ds;
        fb_info.sample_count = m_current_msaa;

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

    rhi::render_pass_handle render_target::create_rp()
    {
        rhi::render_pass_create_info rp_info;

        const bool need_resolve = m_current_msaa > 1;

        auto to_store_op = [need_resolve](rhi::store_op original_op) -> rhi::store_op {
            if (original_op == rhi::store_op::dont_care) {
                return rhi::store_op::dont_care;
            }
            return need_resolve ? rhi::store_op::resolve : rhi::store_op::store;
        };

        uint32 index = 0;
        for (const auto& ci : m_ca_cfg) {
            rhi::color_attachment_info ca_info;
            ca_info.format = ci.format;
            ca_info.load = ci.load;
            ca_info.store = to_store_op(ci.store);
            ca_info.resolve_target = need_resolve ? m_dst_cl[index] : rhi::texture_handle{};
            ca_info.clear_value[0] = ci.clear_value[0];
            ca_info.clear_value[1] = ci.clear_value[1];
            ca_info.clear_value[2] = ci.clear_value[2];
            ca_info.clear_value[3] = ci.clear_value[3];
            ++index;

            rp_info.color_attachments.push_back(ca_info);
        }

        rhi::depth_stencil_attachment_info dsca_info;
        dsca_info.format = combine_depth_stencil_formats(m_da_cfg.format, m_sa_cfg.format);
        dsca_info.depth_load = m_da_cfg.load;
        dsca_info.depth_store = to_store_op(m_da_cfg.store);
        dsca_info.depth_clear_value = m_da_cfg.clear_value;
        dsca_info.stencil_load = m_sa_cfg.load;
        dsca_info.stencil_store = to_store_op(m_sa_cfg.store);
        dsca_info.stencil_clear_value = m_sa_cfg.clear_value;
        dsca_info.resolve_target = need_resolve ? m_dst_ds : rhi::texture_handle{};

        rp_info.depth_stencil_attachment = dsca_info;

        auto rp = m_gdevice->create_render_pass(rp_info);
        if (!rp) {
            ::logger.fatal("Failed to create render pass.");
            return {};
        }

        return rp;
    }

} // namespace tavros::renderer

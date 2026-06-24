#pragma once

#include <tavros/core/fixed_string.hpp>
#include <tavros/tef/workspace.hpp>
#include <tavros/tef/schema.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

namespace tavros::renderer
{

    class render_target_desc
    {
    public:
        struct color_attachment_config
        {
            core::fixed_string<63> name = "";
            rhi::pixel_format      format = rhi::pixel_format::none;
            rhi::load_op           load = rhi::load_op::dont_care;
            rhi::store_op          store = rhi::store_op::dont_care;
            float                  clear_value[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        };

        using color_attachments_config = core::fixed_vector<color_attachment_config, rhi::k_max_color_attachments>;

        struct depth_attachment_config
        {
            rhi::pixel_format format = rhi::pixel_format::none;
            rhi::load_op      load = rhi::load_op::dont_care;
            rhi::store_op     store = rhi::store_op::dont_care;
            float             clear_value = 1.0f;
        };

        struct stencil_attachment_config
        {
            rhi::pixel_format format = rhi::pixel_format::none;
            rhi::load_op      load = rhi::load_op::dont_care;
            rhi::store_op     store = rhi::store_op::dont_care;
            uint32            clear_value = 0;
        };

    public:
        render_target_desc() noexcept = default;

        render_target_desc(
            core::string_view                rt_name,
            const color_attachments_config&  ca,
            const depth_attachment_config&   da,
            const stencil_attachment_config& sa
        ) noexcept
            : m_name(rt_name)
            , m_color_configs(ca)
            , m_depth_config(da)
            , m_stencil_config(sa)
        {
        }

        core::string_view name() const noexcept
        {
            return m_name;
        }

        const color_attachments_config& color_attachments() const noexcept
        {
            return m_color_configs;
        }

        const depth_attachment_config& depth_attachment() const noexcept
        {
            return m_depth_config;
        }

        const stencil_attachment_config& stencil_attachment() const noexcept
        {
            return m_stencil_config;
        }

    private:
        core::short_string        m_name;
        color_attachments_config  m_color_configs;
        depth_attachment_config   m_depth_config;
        stencil_attachment_config m_stencil_config;
    };

} // namespace tavros::renderer

namespace tavros::tef
{
    template<>
    struct schema<tavros::renderer::render_target_desc>
    {
        static void serialize(node* n, const tavros::renderer::render_target_desc& in, core::diagnostics& ds) noexcept;
        static void deserialize(const node* n, tavros::renderer::render_target_desc& out, core::diagnostics& ds) noexcept;
    };
} // namespace tavros::tef

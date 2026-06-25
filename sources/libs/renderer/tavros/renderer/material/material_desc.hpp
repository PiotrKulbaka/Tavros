#pragma once

#include <tavros/core/fixed_string.hpp>
#include <tavros/tef/workspace.hpp>
#include <tavros/tef/schema.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

namespace tavros::renderer
{

    class material_desc
    {
    public:
        struct shader_config
        {
            core::fixed_path vertex_shader_path = "";
            core::fixed_path fragment_shader_path = "";
        };

        struct color_attachment_state_config
        {
            core::short_string           name = "";
            core::flags<rhi::color_mask> mask = rhi::k_rgba_color_mask;
            rhi::blend_state             blend;
        };

        using color_attachments_state_config = core::fixed_vector<color_attachment_state_config, rhi::k_max_color_attachments>;

        struct depth_attachment_state_config
        {
            bool            test_enabled = false;
            bool            write_enabled = false;
            rhi::compare_op compare_op = rhi::compare_op::always;
        };

        struct stencil_attachment_state_config
        {
            bool               test_enabled = false;
            rhi::stencil_state front;
            rhi::stencil_state back;
        };

        struct topology_config
        {
            rhi::primitive_topology topology = rhi::primitive_topology::triangles;
        };

        using rasterizer_config = rhi::rasterizer_state;

    public:
        material_desc() noexcept
        {
        }

        material_desc(
            core::string_view                      mt_name,
            const shader_config&                   sc,
            const color_attachments_state_config&  ca,
            const depth_attachment_state_config&   da,
            const stencil_attachment_state_config& sa,
            const topology_config&                 tc,
            const rasterizer_config&               rc
        ) noexcept
            : m_name(mt_name)
            , m_shader_config(sc)
            , m_color_configs(ca)
            , m_depth_config(da)
            , m_stencil_config(sa)
            , m_topology_config(tc)
            , m_rasterizer_config(rc)
        {
        }

        ~material_desc() noexcept = default;

        core::string_view name() const noexcept
        {
            return m_name;
        }

        const shader_config& shaders() const noexcept
        {
            return m_shader_config;
        }

        const color_attachments_state_config& color_attachment_states() const noexcept
        {
            return m_color_configs;
        }

        const depth_attachment_state_config& depth_attachment_state() const noexcept
        {
            return m_depth_config;
        }

        const stencil_attachment_state_config& stencil_attachment_state() const noexcept
        {
            return m_stencil_config;
        }

        const topology_config& topology() const noexcept
        {
            return m_topology_config;
        }

        const rasterizer_config& rasterizer() const noexcept
        {
            return m_rasterizer_config;
        }

    private:
        core::short_string              m_name;
        shader_config                   m_shader_config;
        color_attachments_state_config  m_color_configs;
        depth_attachment_state_config   m_depth_config;
        stencil_attachment_state_config m_stencil_config;
        topology_config                 m_topology_config;
        rasterizer_config               m_rasterizer_config;
    };

} // namespace tavros::renderer

namespace tavros::tef
{
    template<>
    struct schema<tavros::renderer::material_desc>
    {
        static void serialize(node* n, const tavros::renderer::material_desc& in, core::diagnostics& ds) noexcept;
        static void deserialize(const node* n, tavros::renderer::material_desc& out, core::diagnostics& ds) noexcept;
    };
} // namespace tavros::tef

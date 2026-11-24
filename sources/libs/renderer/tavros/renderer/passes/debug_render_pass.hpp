#pragma once

#include <tavros/renderer/frame/render_pass.hpp>

namespace tavros::renderer
{

    class debug_render_pass final : public render_pass
    {
    public:
        debug_render_pass(render_system* rs);
        ~debug_render_pass() noexcept override;

        void upload(frame_context& ctx) override;
        void execute(frame_context& ctx) override;

    private:
        static constexpr uint32 k_depth_mode_count = 3; // none, depth_test, depth_write
        static constexpr uint32 k_topology_count = 3;   // triangles, lines, points
        static constexpr uint32 k_material_count = k_depth_mode_count * k_topology_count;

        struct draw_call_t
        {
            uint32              first = 0;    // first vertex in m_vertices
            uint32              count = 0;    // number of vertices
            uint32              material = 0; // index in m_materials
            rhi::texture_handle tex;
        };

    private:
        render_system* m_rs = nullptr;

        // depth mode * 3 + topology
        material_ref                   m_materials[k_material_count];
        core::vector<debug_vertex3d_t> m_vertices;
        core::vector<uint32>           m_order;
        core::vector<draw_call_t>      m_draws;

        rhi::bind_buffer_info m_bind_buffers[4];
        rhi::buffer_binding   m_vertex_buffer_view;
        rhi::texture_handle   m_white_tex;
    };

} // namespace tavros::renderer
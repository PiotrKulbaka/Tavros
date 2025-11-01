#pragma once

#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/core/containers/vector.hpp>

namespace tavros::renderer
{

    class debug_renderer
    {
    public:
        debug_renderer();
        ~debug_renderer();

        void begin_frame(const math::mat4& orto_view_proj, const math::mat4& persp_view_proj);
        void end_frame();

        void draw_point2d(math::vec2 p, float point_size, math::color color);
        void draw_line2d(math::vec2 p1, math::vec2 p2, math::color color);
        void draw_line2d(math::vec2 p1, math::color color1, math::vec2 p2, math::color color2);
        void draw_box2d(geometry::aabb2 rect, math::color color);
        void fill_box2d(geometry::aabb2 rect, math::color color);
        void draw_oriented_box2d(geometry::obb2 box, math::color color);
        void fill_oriented_box2d(geometry::obb2 box, math::color color);

        void draw_point3d(const math::vec3& p, float point_size, const math::color& color);
        void draw_line3d(const math::vec3& p1, const math::vec3& p2, const math::color& color);
        void draw_line3d(const math::vec3& p1, const math::color& color1, const math::vec3& p2, const math::color& color2);
        void draw_box3d(const geometry::aabb3& box, const math::color& color);
        void draw_bezier_curve3d(const math::vec3& start, const math::vec3& p2, const math::vec3& p3, const math::vec3& finish, uint32 segments, const math::color& color_start, const math::color& color_finish, bool draw_edges = false, float segment_point_size = 0.0f);
        void fill_box3d(const geometry::aabb3& box, const math::color& color);
        void draw_oriented_box3d(const geometry::obb3& box, const math::color& color);
        void fill_oriented_box3d(const geometry::obb3& box, const math::color& color);

        void update(rhi::graphics_device* gdevice);
        void render(rhi::command_queue* cmds);

    private:
        void push_line2d(const math::vec3& p1, const math::vec3& p2, const math::color& color);
        void push_quad2d(const math::vec3& p1, const math::vec3& p2, const math::vec3& p3, const math::vec3& p4, const math::color& color);

        void push_line3d(const math::vec3& p1, const math::vec3& p2, const math::color& color);
        void push_tri3d(const math::vec3& p1, const math::vec3& p2, const math::vec3& p3, const math::color& color);
        void push_quad3d(const math::vec3& p1, const math::vec3& p2, const math::vec3& p3, const math::vec3& p4, const math::color& color);

        struct draw_call_view
        {
            rhi::pipeline_handle       pipeline;
            rhi::shader_binding_handle shader_binding;
            rhi::geometry_handle       geometry;
            uint32                     vertex_count = 0;
            uint32                     first_vertex = 0;
        };

        void draw(rhi::command_queue* cmds, const draw_call_view& call) const;

    private:
        struct vert3d_params
        {
            math::vec3  pos;
            float       point_size = 1.0f;
            math::color color;
        };

        struct line3d_params
        {
            vert3d_params p1;
            vert3d_params p2;
        };

        core::vector<vert3d_params> m_points2d;
        core::vector<vert3d_params> m_lines2d;
        core::vector<vert3d_params> m_tris2d;
        core::vector<vert3d_params> m_points3d;
        core::vector<vert3d_params> m_lines3d;
        core::vector<vert3d_params> m_tris3d;

        math::mat4 m_orto;
        math::mat4 m_persp;

        bool                       m_gpu_resources_created = false;
        rhi::buffer_handle         m_scene_params_buffer;
        rhi::buffer_handle         m_draw_geometry_buffer;
        rhi::geometry_handle       m_geometry_binding;
        rhi::shader_binding_handle m_scene_orto_binding;
        rhi::shader_binding_handle m_scene_persp_binding;

        draw_call_view m_draw_points2d;
        draw_call_view m_draw_lines2d;
        draw_call_view m_draw_tris2d;
        draw_call_view m_draw_points3d;
        draw_call_view m_draw_lines3d;
        draw_call_view m_draw_tris3d;
    };

} // namespace tavros::renderer

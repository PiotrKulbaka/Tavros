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

        void init(rhi::graphics_device* gdevice);
        void shutdown();

        void begin_frame(const math::mat4& orto_view_proj, const math::mat4& persp_view_proj);
        void end_frame();

        void draw_point2d(math::vec2 p, float point_size, math::color color);
        void draw_line2d(math::vec2 p1, math::vec2 p2, math::color color);
        void draw_line2d(math::vec2 p1, math::color color1, math::vec2 p2, math::color color2);
        void draw_bezier_curve2d(const math::vec2& start, const math::vec2& p2, const math::vec2& p3, const math::vec2& finish, uint32 segments, const math::color& color_start, const math::color& color_finish, bool draw_edges = false, float segment_point_size = 0.0f);
        void draw_box2d(geometry::aabb2 rect, math::color color);
        void fill_box2d(geometry::aabb2 rect, math::color color);
        void draw_oriented_box2d(geometry::obb2 box, math::color color);
        void fill_oriented_box2d(geometry::obb2 box, math::color color);

        void draw_point3d(const math::vec3& p, float point_size, const math::color& color);
        void draw_line3d(const math::vec3& p1, const math::vec3& p2, const math::color& color);
        void draw_line3d(const math::vec3& p1, const math::color& color1, const math::vec3& p2, const math::color& color2);
        void draw_bezier_curve3d(const math::vec3& start, const math::vec3& p2, const math::vec3& p3, const math::vec3& finish, uint32 segments, const math::color& color_start, const math::color& color_finish, bool draw_edges = false, float segment_point_size = 0.0f);
        void draw_box3d(const geometry::aabb3& box, const math::color& color);
        void fill_box3d(const geometry::aabb3& box, const math::color& color);
        void draw_oriented_box3d(const geometry::obb3& box, const math::color& color);
        void fill_oriented_box3d(const geometry::obb3& box, const math::color& color);

        void fill_sphere(const math::vec3& center, float radius, const math::color& color);

        void update();
        void render(rhi::command_queue* cmds);

    private:
        bool create_shader_bindings();
        bool create_pipelines();
        bool create_static_geom();
        bool create_batch_geom();

        void destroy_all();

        void push_line2d(const math::vec3& p1, const math::vec3& p2, const math::color& color);
        void push_quad2d(const math::vec3& p1, const math::vec3& p2, const math::vec3& p3, const math::vec3& p4, const math::color& color);

        void push_line3d(const math::vec3& p1, const math::vec3& p2, const math::color& color);
        void push_tri3d(const math::vec3& p1, const math::vec3& p2, const math::vec3& p3, const math::color& color);
        void push_quad3d(const math::vec3& p1, const math::vec3& p2, const math::vec3& p3, const math::vec3& p4, const math::color& color);


        struct mesh_view
        {
            uint32 first_vertex = 0;
            uint32 vertex_count = 0;
            uint32 first_index = 0;
            uint32 index_count = 0;
            uint32 instance_count = 0;
        };


    private:
        struct vert3d_pos_norm
        {
            math::vec3 pos;
            math::vec3 norm;
        };

        struct vert3d_instance_data
        {
            math::vec4 color;
            math::mat4 model;
        };

        struct vert3d_params
        {
            math::vec3  pos;
            float       point_size = 1.0f;
            math::color color;
        };

        core::vector<vert3d_params> m_points2d;
        core::vector<vert3d_params> m_lines2d;
        core::vector<vert3d_params> m_tris2d;
        core::vector<vert3d_params> m_points3d;
        core::vector<vert3d_params> m_lines3d;
        core::vector<vert3d_params> m_tris3d;

        core::vector<vert3d_instance_data> m_spheres;

        math::mat4 m_orto;
        math::mat4 m_persp;

        rhi::graphics_device* m_gdevice;
        bool                  m_gpu_resources_created = false;

        // Scene buffer binding
        rhi::buffer_handle         m_scene_params_buffer;
        rhi::shader_binding_handle m_scene_orto_binding;
        rhi::shader_binding_handle m_scene_persp_binding;

        // Batching geom
        rhi::buffer_handle   m_batch_geom_buffer;
        rhi::geometry_handle m_batch_geom_binding;

        // Instanced geom
        rhi::buffer_handle   m_inst_stream_data;
        rhi::buffer_handle   m_static_verts_buffer;
        rhi::buffer_handle   m_static_inds_buffer;
        rhi::geometry_handle m_static_geom_binding;

        // Meshes draw info
        mesh_view m_draw_cube_mesh_info;
        mesh_view m_draw_icosphere_mesh_info;
        mesh_view m_draw_points3d_info;
        mesh_view m_draw_lines3d_info;
        mesh_view m_draw_tris3d_info;
        mesh_view m_draw_points2d_info;
        mesh_view m_draw_lines2d_info;
        mesh_view m_draw_tris2d_info;

        // Pipelines
        rhi::pipeline_handle m_inst_mesh_pipeline;
        rhi::pipeline_handle m_inst_wireframe_mesh_pipeline;
        rhi::pipeline_handle m_points3d_pipeline;
        rhi::pipeline_handle m_lines3d_pipeline;
        rhi::pipeline_handle m_tris3d_pipeline;
        rhi::pipeline_handle m_points2d_pipeline;
        rhi::pipeline_handle m_lines2d_pipeline;
        rhi::pipeline_handle m_tris2d_pipeline;
    };

} // namespace tavros::renderer

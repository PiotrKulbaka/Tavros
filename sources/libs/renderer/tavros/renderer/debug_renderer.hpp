#pragma once

#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>
#include <tavros/core/memory/mallocator.hpp>

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/core/containers/vector.hpp>

#include <tavros/text/font/font.hpp>
#include <tavros/text/text_layout.hpp>

namespace tavros::renderer
{

    class debug_renderer : core::noncopyable
    {
    public:
        enum class draw_mode
        {
            points,
            edges,
            faces,
        };

        enum class vertical_align
        {
            top,
            center,
            bottom,
        };

        struct text_layout
        {
            text::text_align text_align = text::text_align::left;
            vertical_align   vert_align = vertical_align::top;
            float            line_spacing = 1.0f;
        };

        debug_renderer();
        ~debug_renderer();

        void init(rhi::graphics_device* gdevice);
        void shutdown();

        void set_point_size(float size);

        void begin_frame(const math::mat4& view_orto_proj, const math::mat4& view_persp_proj);
        void end_frame();

        void point2d(math::vec2 p, float point_size, math::color color);
        void line2d(math::vec2 p1, math::vec2 p2, math::color color);
        void line2d(math::vec2 p1, math::color color1, math::vec2 p2, math::color color2);
        void bezier_curve2d(const math::vec2& start, const math::vec2& p2, const math::vec2& p3, const math::vec2& finish, uint32 segments, const math::color& color_start, const math::color& color_finish, bool draw_edges = false, float segment_point_size = 0.0f);
        void tri2d(math::vec2 p1, math::vec2 p2, math::vec2 p3, math::color color, draw_mode mode = draw_mode::faces);
        void box2d(geometry::aabb2 rect, math::color color, draw_mode mode = draw_mode::faces);
        void box2d(geometry::obb2 box, math::color color, draw_mode mode = draw_mode::faces);
        void draw_text2d(core::string_view text, float text_size, text_layout layout_params, geometry::aabb2 rect, math::color color);

        void point3d(const math::vec3& p, float point_size, const math::color& color);
        void line3d(const math::vec3& p1, const math::vec3& p2, const math::color& color);
        void line3d(const math::vec3& p1, const math::color& color1, const math::vec3& p2, const math::color& color2);
        void bezier_curve3d(const math::vec3& start, const math::vec3& p2, const math::vec3& p3, const math::vec3& finish, uint32 segments, const math::color& color_start, const math::color& color_finish, bool draw_edges = false, float segment_point_size = 0.0f);
        void tri3d(math::vec3 p1, math::vec3 p2, math::vec3 p3, math::color color, draw_mode mode = draw_mode::faces);
        void box3d(const geometry::aabb3& box, const math::color& color, draw_mode mode = draw_mode::faces);
        void box3d(const geometry::obb3& box, const math::color& color, draw_mode mode = draw_mode::faces);
        void sphere3d(const geometry::sphere& sph, const math::color& color, draw_mode mode = draw_mode::faces);

        void update();
        void render(rhi::command_queue* cmds);

    private:
        struct mesh_view
        {
            uint32 first_vertex = 0;
            uint32 vertex_count = 0;
            uint32 first_index = 0;
            uint32 index_count = 0;
            uint32 instance_count = 0;
            uint32 first_instance = 0;
        };

        void draw(rhi::command_queue* cmds, const mesh_view& mesh);

        bool create_shader_bindings();
        bool create_pipelines();
        bool create_static_geom();
        bool create_batch_geom();
        bool create_font_atlas();

        void destroy_all();

    private:
        struct xyz_norm_t
        {
            math::vec3 pos;
            math::vec3 norm;
        };

        struct instance_data_t
        {
            math::color color;
            math::vec4  uv1_uv2;
            math::mat4  model;
        };

        struct xyz_sz_cl_t
        {
            math::vec3  pos;
            float       point_size = 1.0f;
            math::color color;
        };

        core::mallocator m_alc;

        core::vector<xyz_sz_cl_t> m_points2d;
        core::vector<xyz_sz_cl_t> m_lines2d;
        core::vector<xyz_sz_cl_t> m_tris2d;
        core::vector<xyz_sz_cl_t> m_points3d;
        core::vector<xyz_sz_cl_t> m_lines3d;
        core::vector<xyz_sz_cl_t> m_tris3d;

        core::vector<instance_data_t> m_spheres;
        core::vector<instance_data_t> m_sphere_wireframes;
        core::vector<instance_data_t> m_cubes;

        core::vector<instance_data_t> m_text;

        struct scene_t
        {
            math::mat4 proj;
        };

        struct memory_layout_t
        {
            size_t offset = 0;
            size_t size = 0;
        };

        memory_layout_t m_2d_scene_mem_layout;
        scene_t         m_2d_scene;

        memory_layout_t m_3d_scene_mem_layout;
        scene_t         m_3d_scene;

        float m_point_size;

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
        mesh_view m_draw_icosphere_wireframe_info;
        mesh_view m_draw_text_info;
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
        rhi::pipeline_handle m_text2d_pipeline;

        rhi::texture_handle        m_font_atlas;
        rhi::sampler_handle        m_font_sampler;
        rhi::shader_binding_handle m_font_binding;
        rhi::buffer_handle         m_stage;
        bool                       m_need_upload_texture;
        rhi::texture_copy_region   m_texture_copy_rgn;
        math::vec2                 m_atlas_texture_size;

        core::shared_ptr<text::font> m_font;
    };

} // namespace tavros::renderer

#include <tavros/renderer/debug_renderer.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{

    const char* vertex_shader_source = R"(
#version 430 core

layout (location = 0) in vec4 a_pos_pt_size; // pos: xyz, w: point size
layout (location = 1) in vec4 a_color;

layout (std140, binding = 0) uniform Scene
{
    mat4 u_proj; // perspective or orthogonal
    mat4 u_view;
    mat4 u_view_proj;
};

out vec4 v_color;

void main()
{
    gl_PointSize = a_pos_pt_size.w;
    v_color = a_color;
    gl_Position = u_proj * vec4(a_pos_pt_size.xyz, 1.0);
}
)";

    const char* fragment_shader_source = R"(
#version 430 core

in vec4 v_color;
out vec4 frag_color;

void main()
{
    frag_color = v_color;
}
)";

    namespace r = tavros::renderer::rhi;

    tavros::core::logger logger("debug_renderer");

    r::pipeline_handle create_debug_renderer_pipeline(r::graphics_device* gdevice, r::shader_handle vs, r::shader_handle fs, bool z_test, r::polygon_mode r_mode, r::primitive_topology r_topo)
    {
        tavros::renderer::rhi::pipeline_create_info pipeline_info;
        pipeline_info.attributes.push_back({r::attribute_type::vec4, r::attribute_format::f32, false, 0});
        pipeline_info.attributes.push_back({r::attribute_type::vec4, r::attribute_format::f32, false, 1});
        pipeline_info.shaders.push_back(vs);
        pipeline_info.shaders.push_back(fs);
        pipeline_info.depth_stencil.depth_test_enable = z_test;
        pipeline_info.depth_stencil.depth_write_enable = z_test;
        pipeline_info.depth_stencil.depth_compare = r::compare_op::less;
        pipeline_info.rasterizer.cull = r::cull_face::back;
        pipeline_info.rasterizer.face = r::front_face::counter_clockwise;
        pipeline_info.rasterizer.polygon = r_mode;
        pipeline_info.topology = r_topo;
        pipeline_info.blend_states.push_back({true, r::blend_factor::src_alpha, r::blend_factor::one_minus_src_alpha, r::blend_op::add, r::blend_factor::one, r::blend_factor::one_minus_src_alpha, r::blend_op::add, r::k_rgba_color_mask});

        auto pipeline = gdevice->create_pipeline(pipeline_info);
        return pipeline;
    }

} // namespace

namespace tavros::renderer
{

    debug_renderer::debug_renderer()
    {
    }

    debug_renderer::~debug_renderer()
    {
    }

    void debug_renderer::begin_frame(const math::mat4& orto_view_proj, const math::mat4& persp_view_proj)
    {
        m_points2d.clear();
        m_lines2d.clear();
        m_tris2d.clear();
        m_points3d.clear();
        m_lines3d.clear();
        m_tris3d.clear();

        m_orto = orto_view_proj;
        m_persp = persp_view_proj;
    }

    void debug_renderer::end_frame()
    {
    }

    void debug_renderer::draw_point2d(math::vec2 p, float point_size, math::color color)
    {
        m_points2d.emplace_back(math::vec3(p.x, p.y, 0.0f), point_size, color);
    }

    void debug_renderer::draw_line2d(math::vec2 p1, math::vec2 p2, math::color color)
    {
        m_lines2d.emplace_back(math::vec3(p1.x, p1.y, 0.0f), 0.0f, color);
        m_lines2d.emplace_back(math::vec3(p2.x, p2.y, 0.0f), 0.0f, color);
    }

    void debug_renderer::draw_line2d(math::vec2 p1, math::color color1, math::vec2 p2, math::color color2)
    {
        m_lines2d.emplace_back(math::vec3(p1.x, p1.y, 0.0f), 0.0f, color1);
        m_lines2d.emplace_back(math::vec3(p2.x, p2.y, 0.0f), 0.0f, color2);
    }

    void debug_renderer::draw_box2d(geometry::aabb2 rect, math::color color)
    {
        math::vec3 v00(rect.min.x, rect.min.y, 0.0f);
        math::vec3 v01(rect.min.x, rect.max.y, 0.0f);
        math::vec3 v10(rect.max.x, rect.min.y, 0.0f);
        math::vec3 v11(rect.max.x, rect.max.y, 0.0f);

        push_line2d(v00, v01, color);
        push_line2d(v10, v11, color);
        push_line2d(v00, v10, color);
        push_line2d(v01, v11, color);
    }

    void debug_renderer::fill_box2d(geometry::aabb2 box, math::color color)
    {
        math::vec3 v00(box.min.x, box.min.y, 0.0f);
        math::vec3 v01(box.min.x, box.max.y, 0.0f);
        math::vec3 v10(box.max.x, box.min.y, 0.0f);
        math::vec3 v11(box.max.x, box.max.y, 0.0f);

        push_quad2d(v00, v01, v10, v11, color);
    }

    void debug_renderer::draw_oriented_box2d(geometry::obb2 box, math::color color)
    {
        math::vec3 v00(box.center - box.right * box.half_extents.x - box.up * box.half_extents.y, 0.0f);
        math::vec3 v01(box.center - box.right * box.half_extents.x + box.up * box.half_extents.y, 0.0f);
        math::vec3 v10(box.center + box.right * box.half_extents.x - box.up * box.half_extents.y, 0.0f);
        math::vec3 v11(box.center + box.right * box.half_extents.x + box.up * box.half_extents.y, 0.0f);

        push_line2d(v00, v01, color);
        push_line2d(v10, v11, color);
        push_line2d(v00, v10, color);
        push_line2d(v01, v11, color);
    }

    void debug_renderer::fill_oriented_box2d(geometry::obb2 box, math::color color)
    {
        math::vec3 v00(box.center - box.right * box.half_extents.x - box.up * box.half_extents.y, 0.0f);
        math::vec3 v01(box.center - box.right * box.half_extents.x + box.up * box.half_extents.y, 0.0f);
        math::vec3 v10(box.center + box.right * box.half_extents.x - box.up * box.half_extents.y, 0.0f);
        math::vec3 v11(box.center + box.right * box.half_extents.x + box.up * box.half_extents.y, 0.0f);

        push_quad2d(v00, v01, v10, v11, color);
    }

    void debug_renderer::draw_point3d(const math::vec3& p, float point_size, const math::color& color)
    {
        m_points3d.emplace_back(p, point_size, color);
    }

    void debug_renderer::draw_line3d(const math::vec3& p1, const math::vec3& p2, const math::color& color)
    {
        m_lines3d.emplace_back(p1, 0.0f, color);
        m_lines3d.emplace_back(p2, 0.0f, color);
    }

    void debug_renderer::draw_line3d(const math::vec3& p1, const math::color& color1, const math::vec3& p2, const math::color& color2)
    {
        m_lines3d.emplace_back(p1, 0.0f, color1);
        m_lines3d.emplace_back(p2, 0.0f, color2);
    }

    void debug_renderer::draw_box3d(const geometry::aabb3& box, const math::color& color)
    {
        math::vec3 v000(box.min.x, box.min.y, box.min.z);
        math::vec3 v001(box.min.x, box.min.y, box.max.z);
        math::vec3 v010(box.min.x, box.max.y, box.min.z);
        math::vec3 v011(box.min.x, box.max.y, box.max.z);
        math::vec3 v100(box.max.x, box.min.y, box.min.z);
        math::vec3 v101(box.max.x, box.min.y, box.max.z);
        math::vec3 v110(box.max.x, box.max.y, box.min.z);
        math::vec3 v111(box.max.x, box.max.y, box.max.z);

        // Along X axis
        push_line3d(v000, v100, color);
        push_line3d(v001, v101, color);
        push_line3d(v010, v110, color);
        push_line3d(v011, v111, color);

        // Along Y axis
        push_line3d(v000, v010, color);
        push_line3d(v001, v011, color);
        push_line3d(v100, v110, color);
        push_line3d(v101, v111, color);

        // Along Z axis
        push_line3d(v000, v001, color);
        push_line3d(v010, v011, color);
        push_line3d(v100, v101, color);
        push_line3d(v110, v111, color);
    }

    void debug_renderer::draw_bezier_curve3d(const math::vec3& start, const math::vec3& p2, const math::vec3& p3, const math::vec3& finish, uint32 segments, const math::color& color_start, const math::color& color_finish, bool draw_edges, float segment_point_size)
    {
        if (segments > 1024) {
            segments = 1024;
        }
        if (segments < 1) {
            segments = 1;
        }

        auto bezier_point = [](const math::vec3& p0, const math::vec3& p1, const math::vec3& p2, const math::vec3& p3, float t) -> math::vec3 {
            float u = 1.0f - t;
            float t2 = t * t;
            float u2 = u * u;
            float t3 = t2 * t;
            float u3 = u2 * u;
            return (u3 * p0) + (3.0f * u2 * t * p1) + (3.0f * u * t2 * p2) + (t3 * p3);
        };

        if (draw_edges) {
            push_line3d(start, p2, color_start);
            push_line3d(finish, p3, color_finish);
        }

        math::vec3  prev = start;
        math::color prev_color = color_start;
        bool        draw_segments = segment_point_size > 0.0f;

        if (draw_segments) {
            m_points3d.emplace_back(start, segment_point_size, color_start);
            m_points3d.emplace_back(p2, segment_point_size, color_start);
            m_points3d.emplace_back(p3, segment_point_size, color_finish);
        }

        for (uint32 i = 1; i <= segments; ++i) {
            float      t = static_cast<float>(i) / static_cast<float>(segments);
            math::vec3 cur = bezier_point(start, p2, p3, finish, t);

            // Линейная интерполяция цвета
            math::color cur_color = math::lerp(color_start, color_finish, t);

            // Нарисовать отрезок между prev и curr
            m_lines3d.emplace_back(prev, 0.0f, prev_color);
            m_lines3d.emplace_back(cur, 0.0f, cur_color);

            if (draw_segments) {
                m_points3d.emplace_back(cur, segment_point_size, cur_color);
            }

            prev = cur;
            prev_color = cur_color;
        }
    }

    void debug_renderer::fill_box3d(const geometry::aabb3& box, const math::color& color)
    {
        math::vec3 v000(box.min.x, box.min.y, box.min.z);
        math::vec3 v001(box.min.x, box.min.y, box.max.z);
        math::vec3 v010(box.min.x, box.max.y, box.min.z);
        math::vec3 v011(box.min.x, box.max.y, box.max.z);
        math::vec3 v100(box.max.x, box.min.y, box.min.z);
        math::vec3 v101(box.max.x, box.min.y, box.max.z);
        math::vec3 v110(box.max.x, box.max.y, box.min.z);
        math::vec3 v111(box.max.x, box.max.y, box.max.z);

        push_quad3d(v000, v100, v001, v101, color); // Right XZ plane
        push_quad3d(v010, v011, v110, v111, color); // Left XZ plane
        push_quad3d(v001, v101, v011, v111, color); // Top XY plane
        push_quad3d(v000, v010, v100, v110, color); // Bottom XY plane
        push_quad3d(v000, v001, v010, v011, color); // Back YZ plane
        push_quad3d(v100, v110, v101, v111, color); // Front YZ plane
    }

    void debug_renderer::draw_oriented_box3d(const geometry::obb3& box, const math::color& color)
    {
        math::vec3 v000(box.center - box.forward * box.half_extents.x - box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v001(box.center - box.forward * box.half_extents.x - box.right * box.half_extents.y + box.up * box.half_extents.z);
        math::vec3 v010(box.center - box.forward * box.half_extents.x + box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v011(box.center - box.forward * box.half_extents.x + box.right * box.half_extents.y + box.up * box.half_extents.z);
        math::vec3 v100(box.center + box.forward * box.half_extents.x - box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v101(box.center + box.forward * box.half_extents.x - box.right * box.half_extents.y + box.up * box.half_extents.z);
        math::vec3 v110(box.center + box.forward * box.half_extents.x + box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v111(box.center + box.forward * box.half_extents.x + box.right * box.half_extents.y + box.up * box.half_extents.z);

        // Along X axis
        push_line3d(v000, v100, color);
        push_line3d(v001, v101, color);
        push_line3d(v010, v110, color);
        push_line3d(v011, v111, color);

        // Along Y axis
        push_line3d(v000, v010, color);
        push_line3d(v001, v011, color);
        push_line3d(v100, v110, color);
        push_line3d(v101, v111, color);

        // Along Z axis
        push_line3d(v000, v001, color);
        push_line3d(v010, v011, color);
        push_line3d(v100, v101, color);
        push_line3d(v110, v111, color);
    }

    void debug_renderer::fill_oriented_box3d(const geometry::obb3& box, const math::color& color)
    {
        math::vec3 v000(box.center - box.forward * box.half_extents.x - box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v001(box.center - box.forward * box.half_extents.x - box.right * box.half_extents.y + box.up * box.half_extents.z);
        math::vec3 v010(box.center - box.forward * box.half_extents.x + box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v011(box.center - box.forward * box.half_extents.x + box.right * box.half_extents.y + box.up * box.half_extents.z);
        math::vec3 v100(box.center + box.forward * box.half_extents.x - box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v101(box.center + box.forward * box.half_extents.x - box.right * box.half_extents.y + box.up * box.half_extents.z);
        math::vec3 v110(box.center + box.forward * box.half_extents.x + box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v111(box.center + box.forward * box.half_extents.x + box.right * box.half_extents.y + box.up * box.half_extents.z);

        push_quad3d(v000, v100, v001, v101, color); // Right XZ plane
        push_quad3d(v010, v011, v110, v111, color); // Left XZ plane
        push_quad3d(v001, v101, v011, v111, color); // Top XY plane
        push_quad3d(v000, v010, v100, v110, color); // Bottom XY plane
        push_quad3d(v000, v001, v010, v011, color); // Back YZ plane
        push_quad3d(v100, v110, v101, v111, color); // Front YZ plane
    }

    void debug_renderer::update(rhi::graphics_device* gdevice)
    {
        if (!m_gpu_resources_created) {
            // Create UBO (Scene binding=0 in shader)
            rhi::buffer_create_info ubo_desc{1024ull, rhi::buffer_usage::constant, rhi::buffer_access::cpu_to_gpu};
            m_scene_params_buffer = gdevice->create_buffer(ubo_desc);
            if (!m_scene_params_buffer) {
                ::logger.error("Failed to create ubo (scene)");
                return;
            }

            // Create shader binding for Scene
            rhi::shader_binding_create_info scene_orto_sb_info;
            scene_orto_sb_info.buffer_bindings.push_back({m_scene_params_buffer, 0, 256, 0});
            m_scene_orto_binding = gdevice->create_shader_binding(scene_orto_sb_info);
            if (!m_scene_orto_binding) {
                ::logger.error("Failed to create scene orto binding");
                return;
            }

            rhi::shader_binding_create_info scene_persp_sb_info;
            scene_persp_sb_info.buffer_bindings.push_back({m_scene_params_buffer, 256, 256, 0});
            m_scene_persp_binding = gdevice->create_shader_binding(scene_persp_sb_info);
            if (!m_scene_persp_binding) {
                ::logger.error("Failed to create scene orto binding");
                return;
            }

            // Geometry buffer
            rhi::buffer_create_info geom_buf_desc{1024ull * 1024ull * 16ull, rhi::buffer_usage::vertex, rhi::buffer_access::cpu_to_gpu};
            m_draw_geometry_buffer = gdevice->create_buffer(geom_buf_desc);
            if (!m_draw_geometry_buffer) {
                ::logger.error("Failed to create draw buffer");
                return;
            }

            // Geometry binding
            rhi::geometry_create_info geom_info;
            geom_info.vertex_buffer_layouts.push_back({m_draw_geometry_buffer, 0, sizeof(vert3d_params)});
            geom_info.attribute_bindings.push_back({0, offsetof(vert3d_params, vert3d_params::pos), 0, rhi::attribute_type::vec4, rhi::attribute_format::f32, false, 0});
            geom_info.attribute_bindings.push_back({0, offsetof(vert3d_params, vert3d_params::color), 0, rhi::attribute_type::vec4, rhi::attribute_format::f32, false, 1});
            geom_info.has_index_buffer = false;

            m_geometry_binding = gdevice->create_geometry(geom_info);
            if (!m_geometry_binding) {
                ::logger.fatal("Failed to create geometry");
                return;
            }

            auto vertex_shader = gdevice->create_shader({vertex_shader_source, rhi::shader_stage::vertex, "main"});
            auto fragment_shader = gdevice->create_shader({fragment_shader_source, rhi::shader_stage::fragment, "main"});

            // Create pipeline for 2d points
            m_draw_points2d.geometry = m_geometry_binding;
            m_draw_points2d.shader_binding = m_scene_orto_binding;
            m_draw_points2d.pipeline = create_debug_renderer_pipeline(gdevice, vertex_shader, fragment_shader, false, rhi::polygon_mode::points, rhi::primitive_topology::points);
            if (!m_draw_points2d.pipeline) {
                ::logger.error("Failed to create rendering pipeline for 2d points");
                return;
            }

            // Create pipeline for 2d lines
            m_draw_lines2d.geometry = m_geometry_binding;
            m_draw_lines2d.shader_binding = m_scene_orto_binding;
            m_draw_lines2d.pipeline = create_debug_renderer_pipeline(gdevice, vertex_shader, fragment_shader, false, rhi::polygon_mode::lines, rhi::primitive_topology::lines);
            if (!m_draw_lines2d.pipeline) {
                ::logger.error("Failed to create rendering pipeline for 2d lines");
                return;
            }

            // Create pipeline for 2d tris
            m_draw_tris2d.geometry = m_geometry_binding;
            m_draw_tris2d.shader_binding = m_scene_orto_binding;
            m_draw_tris2d.pipeline = create_debug_renderer_pipeline(gdevice, vertex_shader, fragment_shader, false, rhi::polygon_mode::fill, rhi::primitive_topology::triangles);
            if (!m_draw_tris2d.pipeline) {
                ::logger.error("Failed to create rendering pipeline for 2d triangles");
                return;
            }

            // Create pipeline for 3d points
            m_draw_points3d.geometry = m_geometry_binding;
            m_draw_points3d.shader_binding = m_scene_persp_binding;
            m_draw_points3d.pipeline = create_debug_renderer_pipeline(gdevice, vertex_shader, fragment_shader, true, rhi::polygon_mode::points, rhi::primitive_topology::points);
            if (!m_draw_points3d.pipeline) {
                ::logger.error("Failed to create rendering pipeline for 3d points");
                return;
            }

            // Create pipeline for 3d lines
            m_draw_lines3d.geometry = m_geometry_binding;
            m_draw_lines3d.shader_binding = m_scene_persp_binding;
            m_draw_lines3d.pipeline = create_debug_renderer_pipeline(gdevice, vertex_shader, fragment_shader, true, rhi::polygon_mode::lines, rhi::primitive_topology::lines);
            if (!m_draw_lines3d.pipeline) {
                ::logger.error("Failed to create rendering pipeline for 3d lines");
                return;
            }

            // Create pipeline for 3d tris
            m_draw_tris3d.geometry = m_geometry_binding;
            m_draw_tris3d.shader_binding = m_scene_persp_binding;
            m_draw_tris3d.pipeline = create_debug_renderer_pipeline(gdevice, vertex_shader, fragment_shader, true, rhi::polygon_mode::fill, rhi::primitive_topology::triangles);
            if (!m_draw_tris3d.pipeline) {
                ::logger.error("Failed to create rendering pipeline for 3d triangles");
                return;
            }

            gdevice->destroy_shader(vertex_shader);
            gdevice->destroy_shader(fragment_shader);

            m_gpu_resources_created = true;
        }

        auto scene_map = gdevice->map_buffer(m_scene_params_buffer);
        scene_map.copy_from(&m_orto, sizeof(m_orto), 0);
        scene_map.copy_from(&m_persp, sizeof(m_persp), 256);
        gdevice->unmap_buffer(m_scene_params_buffer);

        // Upload draw data
        auto geom_map = gdevice->map_buffer(m_draw_geometry_buffer);

        size_t offset = 0;
        uint32 first_vertex = 0;

        {
            // Upload points 2d
            auto vert_count = static_cast<uint32>(m_points2d.size());
            m_draw_points2d.first_vertex = first_vertex;
            m_draw_points2d.vertex_count = vert_count;
            offset += geom_map.copy_from(m_points2d.data(), vert_count * sizeof(vert3d_params), offset);
            first_vertex += vert_count;
        }

        {
            // Upload lines 2d
            auto vert_count = static_cast<uint32>(m_lines2d.size());
            m_draw_lines2d.first_vertex = first_vertex;
            m_draw_lines2d.vertex_count = vert_count;
            offset += geom_map.copy_from(m_lines2d.data(), vert_count * sizeof(vert3d_params), offset);
            first_vertex += vert_count;
        }

        {
            // Upload tris 2d
            auto vert_count = static_cast<uint32>(m_tris2d.size());
            m_draw_tris2d.first_vertex = first_vertex;
            m_draw_tris2d.vertex_count = vert_count;
            offset += geom_map.copy_from(m_tris2d.data(), vert_count * sizeof(vert3d_params), offset);
            first_vertex += vert_count;
        }

        {
            // Upload points 3d
            auto vert_count = static_cast<uint32>(m_points3d.size());
            m_draw_points3d.first_vertex = first_vertex;
            m_draw_points3d.vertex_count = vert_count;
            offset += geom_map.copy_from(m_points3d.data(), vert_count * sizeof(vert3d_params), offset);
            first_vertex += vert_count;
        }

        {
            // Upload lines 3d
            auto vert_count = static_cast<uint32>(m_lines3d.size());
            m_draw_lines3d.first_vertex = first_vertex;
            m_draw_lines3d.vertex_count = vert_count;
            offset += geom_map.copy_from(m_lines3d.data(), vert_count * sizeof(vert3d_params), offset);
            first_vertex += vert_count;
        }

        {
            // Upload tris 3d
            auto vert_count = static_cast<uint32>(m_tris3d.size());
            m_draw_tris3d.first_vertex = first_vertex;
            m_draw_tris3d.vertex_count = vert_count;
            offset += geom_map.copy_from(m_tris3d.data(), vert_count * sizeof(vert3d_params), offset);
            first_vertex += vert_count;
        }

        gdevice->unmap_buffer(m_draw_geometry_buffer);
    }

    void debug_renderer::render(rhi::command_queue* cmds)
    {
        if (!m_gpu_resources_created) {
            ::logger.error("Failed to render. GPU resources not created");
            return;
        }

        draw(cmds, m_draw_points3d);
        draw(cmds, m_draw_lines3d);
        draw(cmds, m_draw_tris3d);
        draw(cmds, m_draw_tris2d);
        draw(cmds, m_draw_lines2d);
        draw(cmds, m_draw_points2d);
    }

    void debug_renderer::push_line2d(const math::vec3& p1, const math::vec3& p2, const math::color& color)
    {
        m_lines2d.emplace_back(p1, 0.0f, color);
        m_lines2d.emplace_back(p2, 0.0f, color);
    }

    void debug_renderer::push_quad2d(const math::vec3& p1, const math::vec3& p2, const math::vec3& p3, const math::vec3& p4, const math::color& color)
    {
        m_tris2d.emplace_back(p1, 0.0f, color);
        m_tris2d.emplace_back(p2, 0.0f, color);
        m_tris2d.emplace_back(p3, 0.0f, color);

        m_tris2d.emplace_back(p3, 0.0f, color);
        m_tris2d.emplace_back(p2, 0.0f, color);
        m_tris2d.emplace_back(p4, 0.0f, color);
    }

    void debug_renderer::push_line3d(const math::vec3& p1, const math::vec3& p2, const math::color& color)
    {
        m_lines3d.emplace_back(p1, 0.0f, color);
        m_lines3d.emplace_back(p2, 0.0f, color);
    }

    void debug_renderer::push_tri3d(const math::vec3& p1, const math::vec3& p2, const math::vec3& p3, const math::color& color)
    {
        m_tris3d.emplace_back(p1, 0.0f, color);
        m_tris3d.emplace_back(p2, 0.0f, color);
        m_tris3d.emplace_back(p3, 0.0f, color);
    }

    void debug_renderer::push_quad3d(const math::vec3& p1, const math::vec3& p2, const math::vec3& p3, const math::vec3& p4, const math::color& color)
    {
        m_tris3d.emplace_back(p1, 0.0f, color);
        m_tris3d.emplace_back(p2, 0.0f, color);
        m_tris3d.emplace_back(p3, 0.0f, color);

        m_tris3d.emplace_back(p3, 0.0f, color);
        m_tris3d.emplace_back(p2, 0.0f, color);
        m_tris3d.emplace_back(p4, 0.0f, color);
    }

    void debug_renderer::draw(rhi::command_queue* cmds, const draw_call_view& call) const
    {
        cmds->bind_pipeline(call.pipeline);
        cmds->bind_geometry(call.geometry);
        cmds->bind_shader_binding(call.shader_binding);
        cmds->draw(call.vertex_count, call.first_vertex);
    }

} // namespace tavros::renderer

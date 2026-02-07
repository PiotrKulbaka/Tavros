#include <tavros/renderer/debug_renderer.hpp>

#include <tavros/renderer/geometry/builtin_geometry_generator.hpp>
#include <tavros/renderer/resources/consola_mono_ttf.hpp>
#include <tavros/text/text_layout.hpp>
#include <tavros/text/rich_line.hpp>
#include <tavros/text/font/truetype_font.hpp>
#include <tavros/text/font/font_atlas.hpp>

#include <tavros/core/raii/scoped_owner.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/compression/compression.hpp>
#include <tavros/core/logger/logger.hpp>

#include <array>

namespace
{

    static const auto simple_geom_vert_shsrc = tavros::core::string_view(R"(
#version 430 core
layout (location = 0) in vec4 a_pos_pt_size; // pos: xyz, w: point size
layout (location = 1) in vec4 a_color;
layout (std140, binding = 0) uniform Scene
{
    mat4 u_view_proj;
};

out vec4 v_color;
void main()
{
    gl_PointSize = a_pos_pt_size.w;
    v_color = a_color;
    gl_Position = u_view_proj * vec4(a_pos_pt_size.xyz, 1.0);
})");

    static const auto simple_geom_frag_shsrc = tavros::core::string_view(R"(
#version 430 core

in vec4 v_color;
out vec4 frag_color;

void main()
{
    frag_color = v_color;
}
)");

    static const auto inst_geom_vert_shsrc = tavros::core::string_view(R"(
#version 430 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_norm;
layout (location = 2) in vec4 a_color; // per instance
layout (location = 4) in mat4 a_model; // per instance

layout (std140, binding = 0) uniform Scene
{
    mat4 u_view_proj;
};

out vec4 v_color;

void main()
{
    v_color = a_color;
    mat4 mvp = u_view_proj * a_model;
    gl_Position = mvp * vec4(a_pos, 1.0);
}
)");

    static const auto inst_geom_frag_shsrc = tavros::core::string_view(R"(
#version 430 core

in vec4 v_color;
out vec4 frag_color;

void main()
{
    frag_color = v_color;
}
)");


    static const auto inst_text_vert_shsrc = tavros::core::string_view(R"(
#version 430 core

layout (location = 2) in vec4 a_color; // per instance
layout (location = 3) in vec4 a_uv1_uv2; // per instance
layout (location = 4) in mat4 a_model; // per instance

layout (std140, binding = 0) uniform Scene
{
    mat4 u_view_proj;
};

const vec2 plane_verts[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

out vec2 v_uv;
out vec4 v_color;

void main()
{
    vec2 local_pos = plane_verts[gl_VertexID % 4];
    v_uv = mix(a_uv1_uv2.xy, a_uv1_uv2.zw, local_pos);

    v_color = a_color;
    mat4 mvp = u_view_proj * a_model;
    gl_Position = mvp * vec4(local_pos, 0.0f, 1.0);
}
)");

    static const auto inst_text_frag_shsrc = tavros::core::string_view(R"(
#version 430 core

layout(binding = 0) uniform sampler2D u_atlas;

in vec4 v_color;
in vec2 v_uv;
out vec4 frag_color;

void main()
{
    float a = texture(u_atlas, v_uv).r;

    float text_th = 0.45;
    float smooth_th = 0.15;

    float text_alpha = smoothstep(text_th, text_th + smooth_th, a);
    float final_alpha = v_color.a * text_alpha; 

    frag_color = vec4(v_color.rgb, final_alpha);
}
)");


    tavros::core::logger logger("debug_renderer");

    template<class T>
    class linear_memory_cursor
    {
    public:
        linear_memory_cursor(uint8* base_ptr, size_t total_bytes)
            : m_ptr(base_ptr)
            , m_offset(0)
            , m_total_bytes(total_bytes)
        {
        }

        tavros::core::buffer_span<T> allocate(size_t count)
        {
            T* ptr = reinterpret_cast<T*>(m_ptr + m_offset * sizeof(T));
            m_offset += count;
            return tavros::core::buffer_span<T>(ptr, count);
        }

        size_t offset() const
        {
            return m_offset;
        }

    private:
        uint8* m_ptr;
        size_t m_offset;
        size_t m_total_bytes;
    };

    constexpr float k_atlas_sdf_size_pix = 4.0f;
    constexpr float k_atlas_font_scale_pix = 128.0f;

} // namespace

namespace tavros::renderer
{

    debug_renderer::debug_renderer()
        : m_gdevice(nullptr)
        , m_point_size(24.0f)
    {
    }

    debug_renderer::~debug_renderer()
    {
        if (m_gpu_resources_created) {
            shutdown();
        }
    }

    void debug_renderer::init(rhi::graphics_device* gdevice)
    {
        if (m_gpu_resources_created) {
            ::logger.error("Failed to init(). Renderer already initialized");
            return;
        }

        m_gdevice = gdevice;

        if (!create_shader_bindings()) {
            destroy_all();
            return;
        }

        if (!create_pipelines()) {
            destroy_all();
            return;
        }

        if (!create_batch_geom()) {
            destroy_all();
            return;
        }

        if (!create_static_geom()) {
            destroy_all();
            return;
        }

        if (!create_font_atlas()) {
            destroy_all();
            return;
        }

        m_gpu_resources_created = true;
    }

    void debug_renderer::shutdown()
    {
        if (!m_gpu_resources_created) {
            ::logger.error("Failed to shutdown(). Renderer not initialized");
            return;
        }

        destroy_all();
        m_gpu_resources_created = false;
    }

    void debug_renderer::set_point_size(float size)
    {
        m_point_size = size;
    }

    void debug_renderer::begin_frame(const math::mat4& view_orto_proj, const math::mat4& view_persp_proj)
    {
        m_points2d.clear();
        m_lines2d.clear();
        m_tris2d.clear();
        m_points3d.clear();
        m_lines3d.clear();
        m_tris3d.clear();

        m_spheres.clear();
        m_sphere_wireframes.clear();
        m_cubes.clear();
        m_text.clear();

        m_2d_scene.proj = view_orto_proj;
        m_3d_scene.proj = view_persp_proj;
    }

    void debug_renderer::end_frame()
    {
    }

    void debug_renderer::point2d(math::vec2 p, float point_size, math::color color)
    {
        m_points2d.emplace_back(math::vec3(p), point_size, color);
    }

    void debug_renderer::line2d(math::vec2 p1, math::vec2 p2, math::color color)
    {
        m_lines2d.emplace_back(p1, 0.0f, color);
        m_lines2d.emplace_back(p2, 0.0f, color);
    }

    void debug_renderer::line2d(math::vec2 p1, math::color color1, math::vec2 p2, math::color color2)
    {
        m_lines2d.emplace_back(p1, 0.0f, color1);
        m_lines2d.emplace_back(p2, 0.0f, color2);
    }

    void debug_renderer::bezier_curve2d(const math::vec2& start, const math::vec2& p2, const math::vec2& p3, const math::vec2& finish, uint32 segments, const math::color& color_start, const math::color& color_finish, bool draw_edges, float segment_point_size)
    {
        if (segments > 4096) {
            segments = 4096;
        }
        if (segments < 1) {
            segments = 1;
        }

        if (draw_edges) {
            line2d(start, p2, color_start);
            line2d(finish, p3, color_finish);
        }

        auto bezier_point = [](const math::vec2& p0, const math::vec2& p1, const math::vec2& p2, const math::vec2& p3, float t) -> math::vec2 {
            float u = 1.0f - t;
            float t2 = t * t;
            float u2 = u * u;
            float t3 = t2 * t;
            float u3 = u2 * u;
            return (u3 * p0) + (3.0f * u2 * t * p1) + (3.0f * u * t2 * p2) + (t3 * p3);
        };

        auto prev = start;
        auto prev_color = color_start;
        bool draw_segments = segment_point_size >= 1.0f;

        if (draw_edges) {
            point2d(p2, segment_point_size, color_start);
            point2d(p3, segment_point_size, color_finish);
        }

        if (draw_segments) {
            point2d(start, segment_point_size, color_start);
        }

        for (uint32 i = 1; i <= segments; ++i) {
            auto t = static_cast<float>(i) / static_cast<float>(segments);
            auto cur = bezier_point(start, p2, p3, finish, t);
            auto cur_color = math::lerp(color_start, color_finish, t);

            line2d(prev, prev_color, cur, cur_color);

            if (draw_segments) {
                point2d(cur, segment_point_size, cur_color);
            }

            prev = cur;
            prev_color = cur_color;
        }
    }

    void debug_renderer::tri2d(math::vec2 p1, math::vec2 p2, math::vec2 p3, math::color color, draw_mode mode)
    {
        switch (mode) {
        case draw_mode::points:
            point2d(p1, m_point_size, color);
            point2d(p2, m_point_size, color);
            point2d(p3, m_point_size, color);
            break;
        case draw_mode::edges:
            line2d(p1, p2, color);
            line2d(p2, p3, color);
            line2d(p3, p1, color);
            break;
        case draw_mode::faces:
            m_tris2d.emplace_back(math::vec3(p1), 0.0f, color);
            m_tris2d.emplace_back(math::vec3(p2), 0.0f, color);
            m_tris2d.emplace_back(math::vec3(p3), 0.0f, color);
            break;
        default:
            TAV_UNREACHABLE();
        }
    }

    void debug_renderer::box2d(geometry::aabb2 rect, math::color color, draw_mode mode)
    {
        math::vec2 v00(rect.min.x, rect.min.y);
        math::vec2 v01(rect.min.x, rect.max.y);
        math::vec2 v10(rect.max.x, rect.min.y);
        math::vec2 v11(rect.max.x, rect.max.y);

        switch (mode) {
        case draw_mode::points:
            point2d(v00, m_point_size, color);
            point2d(v01, m_point_size, color);
            point2d(v10, m_point_size, color);
            point2d(v11, m_point_size, color);
            break;
        case draw_mode::edges:
            line2d(v00, v01, color);
            line2d(v10, v11, color);
            line2d(v00, v10, color);
            line2d(v01, v11, color);
            break;
        case draw_mode::faces:
            tri2d(v00, v01, v10, color);
            tri2d(v10, v01, v11, color);
            break;
        default:
            TAV_UNREACHABLE();
        }
    }

    void debug_renderer::box2d(geometry::obb2 box, math::color color, draw_mode mode)
    {
        math::vec2 v00(box.center - box.right * box.half_extents.x - box.up * box.half_extents.y);
        math::vec2 v01(box.center - box.right * box.half_extents.x + box.up * box.half_extents.y);
        math::vec2 v10(box.center + box.right * box.half_extents.x - box.up * box.half_extents.y);
        math::vec2 v11(box.center + box.right * box.half_extents.x + box.up * box.half_extents.y);

        switch (mode) {
        case draw_mode::points:
            point2d(v00, m_point_size, color);
            point2d(v01, m_point_size, color);
            point2d(v10, m_point_size, color);
            point2d(v11, m_point_size, color);
            break;
        case draw_mode::edges:
            line2d(v00, v01, color);
            line2d(v10, v11, color);
            line2d(v00, v10, color);
            line2d(v01, v11, color);
            break;
        case draw_mode::faces:
            tri2d(v00, v01, v10, color);
            tri2d(v10, v01, v11, color);
            break;
        default:
            TAV_UNREACHABLE();
        }
    }

    void debug_renderer::draw_text2d(core::string_view text, float text_size, text_layout text_layout_p, geometry::aabb2 rect, math::color color)
    {
        text::rich_line line;
        line.set_text(text, m_font.get(), text_size);

        text::layout_params params;
        params.width = rect.size().width;
        params.line_spacing = text_layout_p.line_spacing;
        params.align = text_layout_p.text_align;
        auto  glyphs_bbox = text::text_layout::layout(line.glyphs(), params);
        float pad = k_atlas_sdf_size_pix / k_atlas_font_scale_pix;

        float y_off = 0.0f;
        switch (text_layout_p.vert_align) {
        case vertical_align::top:
            break;
        case vertical_align::center:
            y_off = (rect.size().height - glyphs_bbox.size().height) / 2.0f;
            break;
        case vertical_align::bottom:
            y_off = (rect.size().height - glyphs_bbox.size().height);
            break;
        }

        for (const auto& it : line.glyphs()) {
            instance_data_t d;

            d.color = color;
            d.uv1_uv2 = math::vec4(
                static_cast<float>(it.base.rect.left) / m_atlas_texture_size.width,
                static_cast<float>(it.base.rect.top) / m_atlas_texture_size.height,
                static_cast<float>(it.base.rect.right) / m_atlas_texture_size.width,
                static_cast<float>(it.base.rect.bottom) / m_atlas_texture_size.height
            );

            auto scaled_pad = pad * it.base.glyph_size;
            auto size = it.base.layout.size();
            d.model = math::mat4::scale_translate(size + scaled_pad * 2.0f, rect.min + it.base.layout.min - scaled_pad + math::vec2(0.0f, y_off));

            m_text.emplace_back(d);
        }
    }

    void debug_renderer::point3d(const math::vec3& p, float point_size, const math::color& color)
    {
        m_points3d.emplace_back(p, point_size, color);
    }

    void debug_renderer::line3d(const math::vec3& p1, const math::vec3& p2, const math::color& color)
    {
        m_lines3d.emplace_back(p1, 0.0f, color);
        m_lines3d.emplace_back(p2, 0.0f, color);
    }

    void debug_renderer::line3d(const math::vec3& p1, const math::color& color1, const math::vec3& p2, const math::color& color2)
    {
        m_lines3d.emplace_back(p1, 0.0f, color1);
        m_lines3d.emplace_back(p2, 0.0f, color2);
    }

    void debug_renderer::bezier_curve3d(const math::vec3& start, const math::vec3& p2, const math::vec3& p3, const math::vec3& finish, uint32 segments, const math::color& color_start, const math::color& color_finish, bool draw_edges, float segment_point_size)
    {
        if (segments > 4096) {
            segments = 4096;
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
            line3d(start, p2, color_start);
            line3d(finish, p3, color_finish);
        }

        auto prev = start;
        auto prev_color = color_start;
        bool draw_segments = segment_point_size > 0.0f;

        if (draw_segments) {
            point3d(start, segment_point_size, color_start);
            if (draw_edges) {
                point3d(p2, segment_point_size, color_start);
                point3d(p3, segment_point_size, color_finish);
            }
        }

        for (uint32 i = 1; i <= segments; ++i) {
            auto t = static_cast<float>(i) / static_cast<float>(segments);
            auto cur = bezier_point(start, p2, p3, finish, t);
            auto cur_color = math::lerp(color_start, color_finish, t);

            line3d(prev, prev_color, cur, cur_color);

            if (draw_segments) {
                point3d(cur, segment_point_size, cur_color);
            }

            prev = cur;
            prev_color = cur_color;
        }
    }

    void debug_renderer::tri3d(math::vec3 p1, math::vec3 p2, math::vec3 p3, math::color color, draw_mode mode)
    {
        switch (mode) {
        case draw_mode::points:
            point3d(p1, m_point_size, color);
            point3d(p2, m_point_size, color);
            point3d(p3, m_point_size, color);
            break;
        case draw_mode::edges:
            line3d(p1, p2, color);
            line3d(p2, p3, color);
            line3d(p3, p1, color);
            break;
        case draw_mode::faces:
            m_tris3d.emplace_back(p1, 0.0f, color);
            m_tris3d.emplace_back(p2, 0.0f, color);
            m_tris3d.emplace_back(p3, 0.0f, color);
            break;
        default:
            TAV_UNREACHABLE();
        }
    }

    void debug_renderer::box3d(const geometry::aabb3& box, const math::color& color, draw_mode mode)
    {
        if (mode == draw_mode::faces) {
            m_cubes.emplace_back(color, math::vec4(), math::mat4::scale_translate(box.size() / 2.0f, box.center()));
            return;
        }

        math::vec3 v000(box.min.x, box.min.y, box.min.z);
        math::vec3 v001(box.min.x, box.min.y, box.max.z);
        math::vec3 v010(box.min.x, box.max.y, box.min.z);
        math::vec3 v011(box.min.x, box.max.y, box.max.z);
        math::vec3 v100(box.max.x, box.min.y, box.min.z);
        math::vec3 v101(box.max.x, box.min.y, box.max.z);
        math::vec3 v110(box.max.x, box.max.y, box.min.z);
        math::vec3 v111(box.max.x, box.max.y, box.max.z);

        if (mode == draw_mode::edges) {
            // Along X axis
            line3d(v000, v100, color);
            line3d(v001, v101, color);
            line3d(v010, v110, color);
            line3d(v011, v111, color);

            // Along Y axis
            line3d(v000, v010, color);
            line3d(v001, v011, color);
            line3d(v100, v110, color);
            line3d(v101, v111, color);

            // Along Z axis
            line3d(v000, v001, color);
            line3d(v010, v011, color);
            line3d(v100, v101, color);
            line3d(v110, v111, color);
        } else if (mode == draw_mode::points) {
            point3d(v000, m_point_size, color);
            point3d(v001, m_point_size, color);
            point3d(v010, m_point_size, color);
            point3d(v011, m_point_size, color);
            point3d(v100, m_point_size, color);
            point3d(v101, m_point_size, color);
            point3d(v110, m_point_size, color);
            point3d(v111, m_point_size, color);
        } else {
            TAV_UNREACHABLE();
        }
    }

    void debug_renderer::box3d(const geometry::obb3& box, const math::color& color, draw_mode mode)
    {
        if (mode == draw_mode::faces) {
            m_cubes.emplace_back(color, math::vec4(), box.to_mat());
            return;
        }

        math::vec3 v000(box.center - box.forward * box.half_extents.x - box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v001(box.center - box.forward * box.half_extents.x - box.right * box.half_extents.y + box.up * box.half_extents.z);
        math::vec3 v010(box.center - box.forward * box.half_extents.x + box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v011(box.center - box.forward * box.half_extents.x + box.right * box.half_extents.y + box.up * box.half_extents.z);
        math::vec3 v100(box.center + box.forward * box.half_extents.x - box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v101(box.center + box.forward * box.half_extents.x - box.right * box.half_extents.y + box.up * box.half_extents.z);
        math::vec3 v110(box.center + box.forward * box.half_extents.x + box.right * box.half_extents.y - box.up * box.half_extents.z);
        math::vec3 v111(box.center + box.forward * box.half_extents.x + box.right * box.half_extents.y + box.up * box.half_extents.z);

        if (mode == draw_mode::edges) {
            // Along X axis
            line3d(v000, v100, color);
            line3d(v001, v101, color);
            line3d(v010, v110, color);
            line3d(v011, v111, color);

            // Along Y axis
            line3d(v000, v010, color);
            line3d(v001, v011, color);
            line3d(v100, v110, color);
            line3d(v101, v111, color);

            // Along Z axis
            line3d(v000, v001, color);
            line3d(v010, v011, color);
            line3d(v100, v101, color);
            line3d(v110, v111, color);
        } else if (mode == draw_mode::points) {
            point3d(v000, m_point_size, color);
            point3d(v001, m_point_size, color);
            point3d(v010, m_point_size, color);
            point3d(v011, m_point_size, color);
            point3d(v100, m_point_size, color);
            point3d(v101, m_point_size, color);
            point3d(v110, m_point_size, color);
            point3d(v111, m_point_size, color);
        } else {
            TAV_UNREACHABLE();
        }
    }

    void debug_renderer::sphere3d(const geometry::sphere& sph, const math::color& color, draw_mode mode)
    {
        switch (mode) {
        case draw_mode::points:
            point3d(sph.center, m_point_size, color);
            break;
        case draw_mode::edges:
            m_sphere_wireframes.emplace_back(color, math::vec4(), math::mat4::scale_translate(math::vec3(sph.radius), sph.center));
            break;
        case draw_mode::faces:
            m_spheres.emplace_back(color, math::vec4(), math::mat4::scale_translate(math::vec3(sph.radius), sph.center));
            break;
        default:
            TAV_UNREACHABLE();
        }
    }

    void debug_renderer::update()
    {
        if (!m_gpu_resources_created) {
            ::logger.error("Failed to update(). Renderer not initialized");
            return;
        }

        auto scene_map = m_gdevice->map_buffer(m_scene_params_buffer);
        scene_map.copy_from(&m_2d_scene, m_2d_scene_mem_layout.size, m_2d_scene_mem_layout.offset);
        scene_map.copy_from(&m_3d_scene, m_3d_scene_mem_layout.size, m_3d_scene_mem_layout.offset);
        m_gdevice->unmap_buffer(m_scene_params_buffer);

        // Upload draw data
        auto batch_geom_map = m_gdevice->map_buffer(m_batch_geom_buffer);

        size_t offset = 0;
        uint32 first_vertex = 0;

        {
            // Upload points 2d
            auto vert_count = static_cast<uint32>(m_points2d.size());
            m_draw_points2d_info.first_vertex = first_vertex;
            m_draw_points2d_info.vertex_count = vert_count;
            m_draw_points2d_info.instance_count = 1;
            offset += batch_geom_map.copy_from(m_points2d.data(), vert_count * sizeof(xyz_sz_cl_t), offset);
            first_vertex += vert_count;
        }

        {
            // Upload lines 2d
            auto vert_count = static_cast<uint32>(m_lines2d.size());
            m_draw_lines2d_info.first_vertex = first_vertex;
            m_draw_lines2d_info.vertex_count = vert_count;
            m_draw_lines2d_info.instance_count = 1;
            offset += batch_geom_map.copy_from(m_lines2d.data(), vert_count * sizeof(xyz_sz_cl_t), offset);
            first_vertex += vert_count;
        }

        {
            // Upload tris 2d
            auto vert_count = static_cast<uint32>(m_tris2d.size());
            m_draw_tris2d_info.first_vertex = first_vertex;
            m_draw_tris2d_info.vertex_count = vert_count;
            m_draw_tris2d_info.instance_count = 1;
            offset += batch_geom_map.copy_from(m_tris2d.data(), vert_count * sizeof(xyz_sz_cl_t), offset);
            first_vertex += vert_count;
        }

        {
            // Upload points 3d
            auto vert_count = static_cast<uint32>(m_points3d.size());
            m_draw_points3d_info.first_vertex = first_vertex;
            m_draw_points3d_info.vertex_count = vert_count;
            m_draw_points3d_info.instance_count = 1;
            offset += batch_geom_map.copy_from(m_points3d.data(), vert_count * sizeof(xyz_sz_cl_t), offset);
            first_vertex += vert_count;
        }

        {
            // Upload lines 3d
            auto vert_count = static_cast<uint32>(m_lines3d.size());
            m_draw_lines3d_info.first_vertex = first_vertex;
            m_draw_lines3d_info.vertex_count = vert_count;
            m_draw_lines3d_info.instance_count = 1;
            offset += batch_geom_map.copy_from(m_lines3d.data(), vert_count * sizeof(xyz_sz_cl_t), offset);
            first_vertex += vert_count;
        }

        {
            // Upload tris 3d
            auto vert_count = static_cast<uint32>(m_tris3d.size());
            m_draw_tris3d_info.first_vertex = first_vertex;
            m_draw_tris3d_info.vertex_count = vert_count;
            m_draw_tris3d_info.instance_count = 1;
            offset += batch_geom_map.copy_from(m_tris3d.data(), vert_count * sizeof(xyz_sz_cl_t), offset);
            first_vertex += vert_count;
        }

        m_gdevice->unmap_buffer(m_batch_geom_buffer);

        // Instanced geometry
        auto inst_stream_map = m_gdevice->map_buffer(m_inst_stream_data);

        offset = 0;
        uint32 first_instance = 0;
        {
            // Upload spheres 3d
            auto spheres_count = static_cast<uint32>(m_spheres.size());
            m_draw_icosphere_mesh_info.instance_count = spheres_count;
            m_draw_icosphere_mesh_info.first_instance = first_instance;
            offset += inst_stream_map.copy_from(m_spheres.data(), spheres_count * sizeof(instance_data_t), offset);
            first_instance += spheres_count;
        }

        {
            // Upload spheres 3d (wireframes)
            auto spheres_count = static_cast<uint32>(m_sphere_wireframes.size());
            m_draw_icosphere_wireframe_info.instance_count = spheres_count;
            m_draw_icosphere_wireframe_info.first_instance = first_instance;
            offset += inst_stream_map.copy_from(m_sphere_wireframes.data(), spheres_count * sizeof(instance_data_t), offset);
            first_instance += spheres_count;
        }

        {
            // Upload cubes 3d
            auto cubes_count = static_cast<uint32>(m_cubes.size());
            m_draw_cube_mesh_info.instance_count = cubes_count;
            m_draw_cube_mesh_info.first_instance = first_instance;
            offset += inst_stream_map.copy_from(m_cubes.data(), cubes_count * sizeof(instance_data_t), offset);
            first_instance += cubes_count;
        }

        {
            // Upload text
            auto chars_count = static_cast<uint32>(m_text.size());
            m_draw_text_info.instance_count = chars_count;
            m_draw_text_info.first_instance = first_instance;
            offset += inst_stream_map.copy_from(m_text.data(), chars_count * sizeof(instance_data_t), offset);
            first_instance += chars_count;
        }

        m_gdevice->unmap_buffer(m_inst_stream_data);
    }

    void debug_renderer::render(rhi::command_queue* cmds)
    {
        if (!m_gpu_resources_created) {
            ::logger.error("Failed to render(). Renderer not initialized");
            return;
        }

        if (m_need_upload_texture) {
            cmds->copy_buffer_to_texture(m_stage, m_font_atlas, m_texture_copy_rgn);
            m_need_upload_texture = false;
        }

        // Draw 3D geom (instanced)
        cmds->bind_shader_binding(m_scene_persp_binding);

        cmds->bind_pipeline(m_inst_mesh_pipeline);
        rhi::bind_buffer_info draw_bufs_static[] = {{m_static_verts_buffer, 0}, {m_static_verts_buffer, 0}, {m_inst_stream_data, 0}, {m_inst_stream_data, 0}, {m_inst_stream_data, 0}};
        cmds->bind_vertex_buffers(draw_bufs_static);
        cmds->bind_index_buffer(m_static_inds_buffer, rhi::index_buffer_format::u32);
        draw(cmds, m_draw_cube_mesh_info);
        draw(cmds, m_draw_icosphere_mesh_info);


        // Draw 3D geom (batch)
        cmds->bind_pipeline(m_points3d_pipeline);
        rhi::bind_buffer_info draw_bufs_batch[] = {{m_batch_geom_buffer, 0}, {m_batch_geom_buffer, 0}};
        cmds->bind_vertex_buffers(draw_bufs_batch);
        draw(cmds, m_draw_points3d_info);

        cmds->bind_pipeline(m_lines3d_pipeline);
        cmds->bind_vertex_buffers(draw_bufs_batch);
        draw(cmds, m_draw_lines3d_info);

        cmds->bind_pipeline(m_tris3d_pipeline);
        cmds->bind_vertex_buffers(draw_bufs_batch);
        draw(cmds, m_draw_tris3d_info);

        // Draw 3D wireframe geom (instanced)
        cmds->bind_pipeline(m_inst_wireframe_mesh_pipeline);
        cmds->bind_vertex_buffers(draw_bufs_static);
        cmds->bind_index_buffer(m_static_inds_buffer, rhi::index_buffer_format::u32);
        draw(cmds, m_draw_icosphere_wireframe_info);

        // Draw 2D geom (batch)
        cmds->bind_shader_binding(m_scene_orto_binding);

        cmds->bind_pipeline(m_points2d_pipeline);
        cmds->bind_vertex_buffers(draw_bufs_batch);
        draw(cmds, m_draw_points2d_info);

        cmds->bind_pipeline(m_lines2d_pipeline);
        cmds->bind_vertex_buffers(draw_bufs_batch);
        draw(cmds, m_draw_lines2d_info);

        cmds->bind_pipeline(m_tris2d_pipeline);
        cmds->bind_vertex_buffers(draw_bufs_batch);
        draw(cmds, m_draw_tris2d_info);

        cmds->bind_pipeline(m_text2d_pipeline);
        cmds->bind_shader_binding(m_font_binding);
        rhi::bind_buffer_info draw_bufs_text[] = {{m_inst_stream_data, 0}, {m_inst_stream_data, 0}, {m_inst_stream_data, 0}};
        cmds->bind_vertex_buffers(draw_bufs_text);
        draw(cmds, m_draw_text_info);
    }

    void debug_renderer::draw(rhi::command_queue* cmds, const mesh_view& mesh)
    {
        if (mesh.instance_count) {
            if (mesh.index_count) {
                cmds->draw_indexed(mesh.index_count, mesh.first_index, mesh.first_vertex, mesh.instance_count, mesh.first_instance);
            } else {
                cmds->draw(mesh.vertex_count, mesh.first_vertex, mesh.instance_count, mesh.first_instance);
            }
        }
    }

    bool debug_renderer::create_shader_bindings()
    {
        // Create UBO (Scene binding=0 in shader)
        size_t scene_map_offset = 0;
        {
            size_t aligned_size = math::align_up(sizeof(m_2d_scene), 256);
            m_2d_scene_mem_layout.offset = scene_map_offset;
            m_2d_scene_mem_layout.size = sizeof(m_2d_scene);
            scene_map_offset += aligned_size;
        }

        {
            size_t aligned_size = math::align_up(sizeof(m_3d_scene), 256);
            m_3d_scene_mem_layout.offset = scene_map_offset;
            m_3d_scene_mem_layout.size = sizeof(m_3d_scene);
            scene_map_offset += aligned_size;
        }

        size_t                  scene_buf_size = scene_map_offset;
        rhi::buffer_create_info ubo_desc{scene_buf_size, rhi::buffer_usage::constant, rhi::buffer_access::cpu_to_gpu};
        m_scene_params_buffer = m_gdevice->create_buffer(ubo_desc);
        if (!m_scene_params_buffer) {
            ::logger.error("Failed to create ubo (scene)");
            return false;
        }

        // Create shader bindings for Scene
        rhi::shader_binding_create_info scene_orto_sb_info;
        scene_orto_sb_info.buffer_bindings.push_back({m_scene_params_buffer, 0, 256, 0});
        m_scene_orto_binding = m_gdevice->create_shader_binding(scene_orto_sb_info);
        if (!m_scene_orto_binding) {
            ::logger.error("Failed to create scene orto binding");
            return false;
        }

        rhi::shader_binding_create_info scene_persp_sb_info;
        scene_persp_sb_info.buffer_bindings.push_back({m_scene_params_buffer, 256, 256, 0});
        m_scene_persp_binding = m_gdevice->create_shader_binding(scene_persp_sb_info);
        if (!m_scene_persp_binding) {
            ::logger.error("Failed to create scene persp binding");
            return false;
        }

        return true;
    }

    bool debug_renderer::create_pipelines()
    {
        static const rhi::blend_state bs{true, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask};

        auto create_simple_geom_pipeline = [](rhi::graphics_device* d, rhi::shader_handle vs, rhi::shader_handle fs, bool z_test, rhi::cull_face cull, rhi::polygon_mode r_mode, rhi::primitive_topology r_topo) {
            rhi::pipeline_create_info info;
            info.bindings.push_back({rhi::attribute_type::vec4, rhi::attribute_format::f32, false, 0, sizeof(xyz_sz_cl_t), offsetof(xyz_sz_cl_t, xyz_sz_cl_t::pos), 0});
            info.bindings.push_back({rhi::attribute_type::vec4, rhi::attribute_format::f32, false, 1, sizeof(xyz_sz_cl_t), offsetof(xyz_sz_cl_t, xyz_sz_cl_t::color), 0});
            info.shaders.push_back(vs);
            info.shaders.push_back(fs);
            info.depth_stencil.depth_test_enable = z_test;
            info.depth_stencil.depth_write_enable = z_test;
            info.depth_stencil.depth_compare = rhi::compare_op::less;
            info.rasterizer.cull = cull;
            info.rasterizer.face = rhi::front_face::counter_clockwise;
            info.rasterizer.polygon = r_mode;
            info.topology = r_topo;
            info.blend_states.push_back(bs);
            return d->create_pipeline(info);
        };

        auto create_instanced_geom_pipeline = [](rhi::graphics_device* d, rhi::shader_handle vs, rhi::shader_handle fs, bool z_test, bool z_write, rhi::cull_face cull, rhi::polygon_mode r_mode, rhi::primitive_topology r_topo) {
            tavros::renderer::rhi::pipeline_create_info info;
            info.bindings.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0, sizeof(xyz_norm_t), offsetof(xyz_norm_t, xyz_norm_t::pos), 0});
            info.bindings.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 1, sizeof(xyz_norm_t), offsetof(xyz_norm_t, xyz_norm_t::norm), 0});
            info.bindings.push_back({rhi::attribute_type::vec4, rhi::attribute_format::f32, false, 2, sizeof(instance_data_t), offsetof(instance_data_t, instance_data_t::color), 1});
            info.bindings.push_back({rhi::attribute_type::vec4, rhi::attribute_format::f32, false, 3, sizeof(instance_data_t), offsetof(instance_data_t, instance_data_t::uv1_uv2), 1});
            info.bindings.push_back({rhi::attribute_type::mat4, rhi::attribute_format::f32, false, 4, sizeof(instance_data_t), offsetof(instance_data_t, instance_data_t::model), 1});
            info.shaders.push_back(vs);
            info.shaders.push_back(fs);
            info.depth_stencil.depth_test_enable = z_test;
            info.depth_stencil.depth_write_enable = z_write;
            info.depth_stencil.depth_compare = rhi::compare_op::less;
            info.rasterizer.cull = cull;
            info.rasterizer.face = rhi::front_face::counter_clockwise;
            info.rasterizer.polygon = r_mode;
            info.topology = r_topo;
            info.blend_states.push_back(bs);
            return d->create_pipeline(info);
        };

        auto create_instanced_text_pipeline = [](rhi::graphics_device* d, rhi::shader_handle vs, rhi::shader_handle fs, bool z_test, bool z_write, rhi::cull_face cull, rhi::polygon_mode r_mode, rhi::primitive_topology r_topo) {
            tavros::renderer::rhi::pipeline_create_info info;
            info.bindings.push_back({rhi::attribute_type::vec4, rhi::attribute_format::f32, false, 2, sizeof(instance_data_t), offsetof(instance_data_t, instance_data_t::color), 1});
            info.bindings.push_back({rhi::attribute_type::vec4, rhi::attribute_format::f32, false, 3, sizeof(instance_data_t), offsetof(instance_data_t, instance_data_t::uv1_uv2), 1});
            info.bindings.push_back({rhi::attribute_type::mat4, rhi::attribute_format::f32, false, 4, sizeof(instance_data_t), offsetof(instance_data_t, instance_data_t::model), 1});
            info.shaders.push_back(vs);
            info.shaders.push_back(fs);
            info.depth_stencil.depth_test_enable = z_test;
            info.depth_stencil.depth_write_enable = z_write;
            info.depth_stencil.depth_compare = rhi::compare_op::less;
            info.rasterizer.cull = cull;
            info.rasterizer.face = rhi::front_face::counter_clockwise;
            info.rasterizer.polygon = r_mode;
            info.topology = r_topo;
            info.blend_states.push_back(bs);
            return d->create_pipeline(info);
        };

        {
            auto vsh = core::make_scoped_owner(m_gdevice->create_shader({simple_geom_vert_shsrc, rhi::shader_stage::vertex, "main"}), [&](auto sh) { m_gdevice->destroy_shader(sh); });
            auto fsh = core::make_scoped_owner(m_gdevice->create_shader({simple_geom_frag_shsrc, rhi::shader_stage::fragment, "main"}), [&](auto sh) { m_gdevice->destroy_shader(sh); });

            m_points2d_pipeline = create_simple_geom_pipeline(m_gdevice, *vsh, *fsh, false, rhi::cull_face::off, rhi::polygon_mode::points, rhi::primitive_topology::points);
            if (!m_points2d_pipeline) {
                ::logger.error("Failed to create pipeline for 2d points");
                return false;
            }

            m_lines2d_pipeline = create_simple_geom_pipeline(m_gdevice, *vsh, *fsh, false, rhi::cull_face::off, rhi::polygon_mode::lines, rhi::primitive_topology::lines);
            if (!m_lines2d_pipeline) {
                ::logger.error("Failed to create pipeline for 2d lines");
                return false;
            }

            m_tris2d_pipeline = create_simple_geom_pipeline(m_gdevice, *vsh, *fsh, false, rhi::cull_face::off, rhi::polygon_mode::fill, rhi::primitive_topology::triangles);
            if (!m_tris2d_pipeline) {
                ::logger.error("Failed to create pipeline for 2d triangles");
                return false;
            }

            m_points3d_pipeline = create_simple_geom_pipeline(m_gdevice, *vsh, *fsh, true, rhi::cull_face::off, rhi::polygon_mode::points, rhi::primitive_topology::points);
            if (!m_points3d_pipeline) {
                ::logger.error("Failed to create pipeline for 3d points");
                return false;
            }

            m_lines3d_pipeline = create_simple_geom_pipeline(m_gdevice, *vsh, *fsh, true, rhi::cull_face::off, rhi::polygon_mode::lines, rhi::primitive_topology::lines);
            if (!m_lines3d_pipeline) {
                ::logger.error("Failed to create pipeline for 3d lines");
                return false;
            }

            m_tris3d_pipeline = create_simple_geom_pipeline(m_gdevice, *vsh, *fsh, true, rhi::cull_face::back, rhi::polygon_mode::fill, rhi::primitive_topology::triangles);
            if (!m_tris3d_pipeline) {
                ::logger.error("Failed to create pipeline for 3d triangles");
                return false;
            }
        }

        {
            auto vsh = core::make_scoped_owner(m_gdevice->create_shader({inst_geom_vert_shsrc, rhi::shader_stage::vertex, "main"}), [&](auto sh) { m_gdevice->destroy_shader(sh); });
            auto fsh = core::make_scoped_owner(m_gdevice->create_shader({inst_geom_frag_shsrc, rhi::shader_stage::fragment, "main"}), [&](auto sh) { m_gdevice->destroy_shader(sh); });

            m_inst_mesh_pipeline = create_instanced_geom_pipeline(m_gdevice, *vsh, *fsh, true, true, rhi::cull_face::back, rhi::polygon_mode::fill, rhi::primitive_topology::triangles);
            if (!m_inst_mesh_pipeline) {
                ::logger.error("Failed to create instanced mesh pipeline");
                return false;
            }

            m_inst_wireframe_mesh_pipeline = create_instanced_geom_pipeline(m_gdevice, *vsh, *fsh, false, false, rhi::cull_face::off, rhi::polygon_mode::lines, rhi::primitive_topology::triangles);
            if (!m_inst_wireframe_mesh_pipeline) {
                ::logger.error("Failed to create wireframe instanced mesh pipeline");
                return false;
            }
        }

        {
            auto vsh = core::make_scoped_owner(m_gdevice->create_shader({inst_text_vert_shsrc, rhi::shader_stage::vertex, "main"}), [&](auto sh) { m_gdevice->destroy_shader(sh); });
            auto fsh = core::make_scoped_owner(m_gdevice->create_shader({inst_text_frag_shsrc, rhi::shader_stage::fragment, "main"}), [&](auto sh) { m_gdevice->destroy_shader(sh); });

            m_text2d_pipeline = create_instanced_text_pipeline(m_gdevice, *vsh, *fsh, false, false, rhi::cull_face::off, rhi::polygon_mode::fill, rhi::primitive_topology::triangle_strip);
            if (!m_text2d_pipeline) {
                ::logger.error("Failed to create pipeline for 2d text");
                return false;
            }
        }

        return true;
    }

    bool debug_renderer::create_static_geom()
    {
        uint32 icosphere_subdivisions = 2;
        auto   cube_info = builtin_geometry_generator::cube_info();
        auto   icosphere_info = builtin_geometry_generator::icosphere_info(icosphere_subdivisions);

        size_t total_vertices = cube_info.vertices_count + icosphere_info.vertices_count;
        size_t total_vertices_size_bytes = sizeof(builtin_geometry_vertex) * total_vertices;

        size_t total_indices = cube_info.indices_count + icosphere_info.indices_count;
        size_t total_indices_size_bytes = sizeof(uint32) * total_indices;

        // First of all, allocate buffers
        rhi::buffer_create_info static_vertices_desc{total_vertices_size_bytes, rhi::buffer_usage::vertex, rhi::buffer_access::cpu_to_gpu};
        m_static_verts_buffer = m_gdevice->create_buffer(static_vertices_desc);
        if (!m_static_verts_buffer) {
            ::logger.error("Failed to create static geometry vertices buffer");
            return false;
        }

        rhi::buffer_create_info static_indices_desc{total_indices_size_bytes, rhi::buffer_usage::index, rhi::buffer_access::cpu_to_gpu};
        m_static_inds_buffer = m_gdevice->create_buffer(static_indices_desc);
        if (!m_static_inds_buffer) {
            ::logger.error("Failed to create static geometry indices buffer");
            return false;
        }

        rhi::buffer_create_info stream_data_desc{1024ull * 1024ull * 8ull, rhi::buffer_usage::vertex, rhi::buffer_access::cpu_to_gpu};
        m_inst_stream_data = m_gdevice->create_buffer(stream_data_desc);
        if (!m_inst_stream_data) {
            ::logger.error("Failed to create stream data buffer");
            return false;
        }

        // Load data
        auto                                          map_vertices = m_gdevice->map_buffer(m_static_verts_buffer);
        linear_memory_cursor<builtin_geometry_vertex> vertex_cursor(map_vertices.begin(), map_vertices.size());

        auto                         map_indices = m_gdevice->map_buffer(m_static_inds_buffer);
        linear_memory_cursor<uint32> index_cursor(map_indices.begin(), map_indices.size());

        // Load cube
        {
            m_draw_cube_mesh_info.index_count = cube_info.indices_count;
            m_draw_cube_mesh_info.first_index = index_cursor.offset();
            m_draw_cube_mesh_info.vertex_count = cube_info.vertices_count;
            m_draw_cube_mesh_info.first_vertex = vertex_cursor.offset();
            auto cube_verts = vertex_cursor.allocate(m_draw_cube_mesh_info.vertex_count);
            auto cube_inds = index_cursor.allocate(m_draw_cube_mesh_info.index_count);
            builtin_geometry_generator::gen_cube(cube_verts, cube_inds);
        }

        // Load icosphere
        {
            m_draw_icosphere_mesh_info.index_count = icosphere_info.indices_count;
            m_draw_icosphere_mesh_info.first_index = index_cursor.offset();
            m_draw_icosphere_mesh_info.vertex_count = icosphere_info.vertices_count;
            m_draw_icosphere_mesh_info.first_vertex = vertex_cursor.offset();
            m_draw_icosphere_wireframe_info = m_draw_icosphere_mesh_info;
            auto icosphere_verts = vertex_cursor.allocate(m_draw_icosphere_mesh_info.vertex_count);
            auto icosphere_inds = index_cursor.allocate(m_draw_icosphere_mesh_info.index_count);
            builtin_geometry_generator::gen_icosphere(icosphere_subdivisions, icosphere_verts, icosphere_inds);
        }

        // For text use builtin quad (inside shader)
        m_draw_text_info.index_count = 0;
        m_draw_text_info.first_index = 0;
        m_draw_text_info.vertex_count = 4;
        m_draw_text_info.first_vertex = 0;

        m_gdevice->unmap_buffer(m_static_verts_buffer);
        m_gdevice->unmap_buffer(m_static_inds_buffer);

        return true;
    }

    bool debug_renderer::create_batch_geom()
    {
        // Batch geom buffer
        rhi::buffer_create_info geom_buf_desc{1024ull * 1024ull * 8ull, rhi::buffer_usage::vertex, rhi::buffer_access::cpu_to_gpu};
        m_batch_geom_buffer = m_gdevice->create_buffer(geom_buf_desc);
        if (!m_batch_geom_buffer) {
            ::logger.error("Failed to create batch buffer");
            return false;
        }

        return true;
    }

    bool debug_renderer::create_font_atlas()
    {
        auto ttf_data = core::dynamic_buffer<uint8>(&m_alc);
        ttf_data.reserve(g_consola_mono_ttf_uncompressed_size);
        auto compressed_ttf_data = core::buffer_view<uint8>(g_consola_mono_ttf_compressed_data, g_consola_mono_ttf_compressed_size);
        if (!core::uncompress_data(compressed_ttf_data, ttf_data)) {
            ::logger.fatal("Failed to uncompress ttf");
            return false;
        }

        auto                                 ttf = core::make_shared<text::truetype_font>();
        text::truetype_font::codepoint_range glyph_range = {0, 0xffff};
        ttf->init(std::move(ttf_data), glyph_range);
        if (!ttf->is_init()) {
            ::logger.fatal("Failed to init ttf");
            return false;
        }
        m_font = ttf;

        text::font_atlas atlas;
        atlas.register_font(m_font.get());

        auto atlas_buffer = core::dynamic_buffer<uint8>(&m_alc);
        auto atlas_pixels = atlas.invalidate_old_and_bake_new_atlas(atlas_buffer, k_atlas_font_scale_pix, k_atlas_sdf_size_pix);
        m_atlas_texture_size.set(atlas_pixels.width, atlas_pixels.height);

        rhi::texture_create_info tex_info;
        tex_info.type = rhi::texture_type::texture_2d;
        tex_info.format = rhi::pixel_format::r8un;
        tex_info.width = atlas_pixels.width;
        tex_info.height = atlas_pixels.height;
        tex_info.usage = rhi::k_default_texture_usage;

        m_font_atlas = m_gdevice->create_texture(tex_info);
        if (!m_font_atlas) {
            ::logger.fatal("Failed to create texture");
            return false;
        }

        rhi::sampler_create_info sampler_info;
        sampler_info.filter.mipmap_filter = rhi::mipmap_filter_mode::off;
        sampler_info.filter.min_filter = rhi::filter_mode::linear;
        sampler_info.filter.mag_filter = rhi::filter_mode::linear;
        m_font_sampler = m_gdevice->create_sampler(sampler_info);
        if (!m_font_sampler) {
            ::logger.fatal("Failed to create sampler");
            return false;
        }

        rhi::shader_binding_create_info shader_binding_info;
        shader_binding_info.texture_bindings.push_back({m_font_atlas, m_font_sampler, 0});
        m_font_binding = m_gdevice->create_shader_binding(shader_binding_info);
        if (!m_font_binding) {
            ::logger.fatal("Failed to create shader binding");
            return false;
        }

        rhi::buffer_create_info stage_desc{1024ull * 1024ull, rhi::buffer_usage::stage, rhi::buffer_access::cpu_to_gpu};
        m_stage = m_gdevice->create_buffer(stage_desc);
        if (!m_stage) {
            ::logger.fatal("Failed to create stage buffer");
            return false;
        }

        auto map = m_gdevice->map_buffer(m_stage, 0, atlas_pixels.width * atlas_pixels.height);
        map.copy_from(atlas_pixels.pixels, atlas_pixels.width * atlas_pixels.height);
        m_gdevice->unmap_buffer(m_stage);

        m_texture_copy_rgn.width = atlas_pixels.width;
        m_texture_copy_rgn.height = atlas_pixels.height;
        m_need_upload_texture = true;


        return true;
    }

    void debug_renderer::destroy_all()
    {
        m_gdevice->safe_destroy(m_scene_params_buffer);
        m_gdevice->safe_destroy(m_batch_geom_buffer);
        m_gdevice->safe_destroy(m_inst_stream_data);
        m_gdevice->safe_destroy(m_static_verts_buffer);
        m_gdevice->safe_destroy(m_static_inds_buffer);
        m_gdevice->safe_destroy(m_scene_orto_binding);
        m_gdevice->safe_destroy(m_scene_persp_binding);
        m_gdevice->safe_destroy(m_inst_mesh_pipeline);
        m_gdevice->safe_destroy(m_inst_wireframe_mesh_pipeline);
        m_gdevice->safe_destroy(m_points3d_pipeline);
        m_gdevice->safe_destroy(m_lines3d_pipeline);
        m_gdevice->safe_destroy(m_tris3d_pipeline);
        m_gdevice->safe_destroy(m_points2d_pipeline);
        m_gdevice->safe_destroy(m_lines2d_pipeline);
        m_gdevice->safe_destroy(m_tris2d_pipeline);
        m_gdevice->safe_destroy(m_font_atlas);
        m_gdevice->safe_destroy(m_font_sampler);
        m_gdevice->safe_destroy(m_font_binding);
        m_gdevice->safe_destroy(m_text2d_pipeline);
        m_gdevice->safe_destroy(m_stage);
        m_gdevice = nullptr;
    }

} // namespace tavros::renderer

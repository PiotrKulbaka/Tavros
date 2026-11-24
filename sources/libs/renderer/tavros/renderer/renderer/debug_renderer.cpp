#include <tavros/renderer/renderer/debug_renderer.hpp>

#include <tavros/core/math.hpp>

namespace tavros::renderer
{

    void debug_renderer::set_point_size(float size) noexcept
    {
        m_point_size = size;
    }

    void debug_renderer::set_depth_test(debug_depth_test_mode mode) noexcept
    {
        m_depth_test_mode = mode;
    }

    void debug_renderer::point(math::vec3 p, math::color32 cl)
    {
        point(p, {}, cl);
    }

    void debug_renderer::point(math::vec3 p, math::vec3 n, math::color32 cl)
    {
        m_data.add(
            debug_draw_primitive_t{
                .tex = {},
                .verts = m_data.add_range(
                    debug_vertex3d_t{
                        .pos = math::vec4(p, m_point_size),
                        .normal = n,
                        .color = cl,
                        .uv = math::vec2(0.0f, 0.0f)
                    }
                ),
                .test = m_depth_test_mode
            }
        );
    }

    void debug_renderer::line(math::vec3 p0, math::vec3 p1, math::color32 cl)
    {
        line(p0, {}, p1, {}, cl);
    }

    void debug_renderer::line(math::vec3 p0, math::vec3 n0, math::vec3 p1, math::vec3 n1, math::color32 cl)
    {
        m_data.add(
            debug_draw_primitive_t{
                .tex = {},
                .verts = m_data.add_range(
                    {debug_vertex3d_t{
                         .pos = math::vec4(p0, m_point_size),
                         .normal = n0,
                         .color = cl,
                         .uv = math::vec2(0.0f, 0.0f)
                     },
                     debug_vertex3d_t{
                         .pos = math::vec4(p1, m_point_size),
                         .normal = n1,
                         .color = cl,
                         .uv = math::vec2(0.0f, 0.0f)
                     }}
                ),
                .test = m_depth_test_mode
            }
        );
    }

    void debug_renderer::line(math::vec3 p0, math::color32 cl0, math::vec3 p1, math::color32 cl1)
    {
        line(p0, {}, cl0, p1, {}, cl1);
    }

    void debug_renderer::line(math::vec3 p0, math::vec3 n0, math::color32 cl0, math::vec3 p1, math::vec3 n1, math::color32 cl1)
    {
        m_data.add(
            debug_draw_primitive_t{
                .tex = {},
                .verts = m_data.add_range(
                    {debug_vertex3d_t{
                         .pos = math::vec4(p0, m_point_size),
                         .normal = n0,
                         .color = cl0,
                         .uv = math::vec2(0.0f, 0.0f)
                     },
                     debug_vertex3d_t{
                         .pos = math::vec4(p1, m_point_size),
                         .normal = n1,
                         .color = cl1,
                         .uv = math::vec2(0.0f, 0.0f)
                     }}
                ),
                .test = m_depth_test_mode
            }
        );
    }

    void debug_renderer::triangle(math::vec3 p0, math::vec3 p1, math::vec3 p2, math::color32 cl)
    {
        triangle(p0, {}, p1, {}, p2, {}, cl);
    }

    void debug_renderer::triangle(math::vec3 p0, math::vec3 n0, math::vec3 p1, math::vec3 n1, math::vec3 p2, math::vec3 n2, math::color32 cl)
    {
        m_data.add(
            debug_draw_primitive_t{
                .tex = {},
                .verts = m_data.add_range(
                    {debug_vertex3d_t{
                         .pos = math::vec4(p0, m_point_size),
                         .normal = n0,
                         .color = cl,
                         .uv = math::vec2(0.0f, 0.0f)
                     },
                     debug_vertex3d_t{
                         .pos = math::vec4(p1, m_point_size),
                         .normal = n1,
                         .color = cl,
                         .uv = math::vec2(0.0f, 0.0f)
                     },
                     debug_vertex3d_t{
                         .pos = math::vec4(p2, m_point_size),
                         .normal = n2,
                         .color = cl,
                         .uv = math::vec2(0.0f, 0.0f)
                     }}
                ),
                .test = m_depth_test_mode
            }
        );
    }

    void debug_renderer::triangle(math::vec3 p0, math::color32 cl0, math::vec3 p1, math::color32 cl1, math::vec3 p2, math::color32 cl2)
    {
        triangle(p0, {}, cl0, p1, {}, cl1, p2, {}, cl2);
    }

    void debug_renderer::triangle(math::vec3 p0, math::vec3 n0, math::color32 cl0, math::vec3 p1, math::vec3 n1, math::color32 cl1, math::vec3 p2, math::vec3 n2, math::color32 cl2)
    {
        m_data.add(
            debug_draw_primitive_t{
                .tex = {},
                .verts = m_data.add_range(
                    {debug_vertex3d_t{
                         .pos = math::vec4(p0, m_point_size),
                         .normal = n0,
                         .color = cl0,
                         .uv = math::vec2(0.0f, 0.0f)
                     },
                     debug_vertex3d_t{
                         .pos = math::vec4(p1, m_point_size),
                         .normal = n1,
                         .color = cl1,
                         .uv = math::vec2(0.0f, 0.0f)
                     },
                     debug_vertex3d_t{
                         .pos = math::vec4(p2, m_point_size),
                         .normal = n2,
                         .color = cl2,
                         .uv = math::vec2(0.0f, 0.0f)
                     }}
                ),
                .test = m_depth_test_mode
            }
        );
    }

    debug_render_data_t debug_renderer::take() noexcept
    {
        return std::move(m_data);
    }

} // namespace tavros::renderer

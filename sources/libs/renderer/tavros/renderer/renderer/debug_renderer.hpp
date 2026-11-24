#pragma once

#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>
#include <tavros/core/debug/debug.hpp>
#include <tavros/core/containers/typed_vectors.hpp>
#include <tavros/renderer/rhi/handle.hpp>

namespace tavros::renderer
{

    struct debug_vertex3d_t
    {
        math::vec4    pos;          // xyz + point size in w
        math::vec3    normal;       // normal xyz
        math::color32 color;        // packed rgba
        math::vec2    uv;           // uv coords
        float         _pad0 = 0.0f; // padding to 48 bytes
        float         _pad1 = 0.0f; // padding to 48 bytes
    };

    enum class debug_depth_test_mode : uint8
    {
        none = 0,        // no depth test, no depth write
        depth_test = 1,  // depth test enabled, no depth write
        depth_write = 2, // depth test enabled, depth write enabled
    };

    struct debug_draw_primitive_t
    {
        rhi::texture_handle                tex;
        core::range_view<debug_vertex3d_t> verts; // range of vers (1 - point, 2 - line, 3 - triangle)
        debug_depth_test_mode              test = debug_depth_test_mode::depth_test;
    };

    using debug_render_data_types = core::type_list<
        debug_vertex3d_t,
        debug_draw_primitive_t>;

    using debug_render_data_t = core::basic_typed_vectors<debug_render_data_types>;

    class debug_renderer final : core::noncopyable
    {
    public:
        debug_renderer() = default;
        ~debug_renderer() noexcept = default;

        void set_point_size(float size) noexcept;
        void set_depth_test(debug_depth_test_mode mode) noexcept;

        void point(math::vec3 p, math::color32 cl);
        void point(math::vec3 p, math::vec3 n, math::color32 cl);
        void line(math::vec3 p0, math::vec3 p1, math::color32 cl);
        void line(math::vec3 p0, math::vec3 n0, math::vec3 p1, math::vec3 n1, math::color32 cl);
        void line(math::vec3 p0, math::color32 cl0, math::vec3 p1, math::color32 cl1);
        void line(math::vec3 p0, math::vec3 n0, math::color32 cl0, math::vec3 p1, math::vec3 n1, math::color32 cl1);
        void triangle(math::vec3 p0, math::vec3 p1, math::vec3 p2, math::color32 cl);
        void triangle(math::vec3 p0, math::vec3 n0, math::vec3 p1, math::vec3 n1, math::vec3 p2, math::vec3 n2, math::color32 cl);
        void triangle(math::vec3 p0, math::color32 cl0, math::vec3 p1, math::color32 cl1, math::vec3 p2, math::color32 cl2);
        void triangle(math::vec3 p0, math::vec3 n0, math::color32 cl0, math::vec3 p1, math::vec3 n1, math::color32 cl1, math::vec3 p2, math::vec3 n2, math::color32 cl2);

        [[nodiscard]] debug_render_data_t take() noexcept;

    private:
        debug_render_data_t   m_data;
        float                 m_point_size = 1.0f;
        debug_depth_test_mode m_depth_test_mode = debug_depth_test_mode::depth_test;
    };

} // namespace tavros::renderer
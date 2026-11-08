#pragma once

#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>
#include <tavros/core/debug/debug.hpp>
#include <tavros/core/containers/typed_vectors.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/text/rich_text.hpp>

namespace tavros::renderer
{

    /** @brief A color stop used by gradients. */
    struct color_stop_t
    {
        /// Normalized stop position in the range [0, 1].
        float stop = 0.0f;

        /// Color at the stop position.
        math::color32 color;
    };

    enum class gradient_repeat_type : uint8
    {
        clamp,
        repeat,
        mirror
    };

    struct linear_gradient_t
    {
        core::range_view<color_stop_t> paint;
        math::vec2                     from;
        math::vec2                     to;
        gradient_repeat_type           repeat = gradient_repeat_type::clamp;
    };

    struct radial_gradient_t
    {
        core::range_view<color_stop_t> paint;
        math::vec2                     center;
        float                          radius = 0.0f;
        gradient_repeat_type           repeat = gradient_repeat_type::clamp;
    };

    struct conic_gradient_t
    {
        core::range_view<color_stop_t> paint;
        math::vec2                     center;
        math::angle                    start;
        math::angle                    sweep;
        gradient_repeat_type           repeat = gradient_repeat_type::clamp;
    };

    enum class brush_fill_type : uint8
    {
        solid,
        linear,
        radial,
        conic,
    };

    struct paint_brush_t
    {
        brush_fill_type type = brush_fill_type::solid;
        union
        {
            math::color32                      color; // only with brush_fill_type::solid
            core::item_view<linear_gradient_t> linear_view;
            core::item_view<radial_gradient_t> radial_view;
            core::item_view<conic_gradient_t>  conic_view;
        };
    };

    enum class fill_pattern_type : uint8
    {
        linear,
        rect,
        hexagon,
        brick,
        voronoi,
        cairo,
        stripes,
    };

    struct fill_pattern_t
    {
        fill_pattern_type type = fill_pattern_type::rect;
        math::angle       angle;
        float             size0;
        float             size1;
    };

    struct sprite_draw_t
    {
        math::vec2          center;
        math::vec2          half_x;
        math::vec2          half_y;
        math::vec4          atlas_rect;
        rhi::texture_handle tex;
    };

    struct quad_draw_t
    {
        math::vec2 center;
        math::vec2 half_x;
        math::vec2 half_y;
    };

    struct line_draw_t
    {
        math::vec2 from;
        math::vec2 to;
        float      thickness = 0.0f;
        float      dash = 0.0f;
        float      gap = 0.0f;
    };

    struct superellipse_draw_t
    {
        math::vec2 center;
        math::vec2 half_x;
        math::vec2 half_y;
        float      squash = 0.0f;
    };

    struct rect_draw_t
    {
        math::vec2        center;
        math::vec2        half_x;
        math::vec2        half_y;
        geometry::radius4 radius;
    };

    struct sdf_draw_t
    {
        math::vec2          center;
        math::vec2          half_x;
        math::vec2          half_y;
        math::vec4          atlas_rect;
        rhi::texture_handle tex;
        float               sdf_pad_size = 1.0f;
    };

    enum class shape_type : uint8
    {
        quad,
        line,
        superellipse,
        rect,
        sdf,
    };

    struct draw_shape_t
    {
        shape_type type = shape_type::quad;
        union
        {
            core::item_view<quad_draw_t>         quad_view;
            core::item_view<line_draw_t>         line_view;
            core::item_view<superellipse_draw_t> superellipse_view;
            core::item_view<rect_draw_t>         rect_view;
            core::item_view<sdf_draw_t>          sdf_view;
        };
        float shadow_bias = 0.0f;
        float shadow_radius = 0.0f;
    };

    struct scissor_rect_t
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
    };

    struct ui_draw_command_t
    {
        core::item_view<paint_brush_t>  brush;
        core::item_view<fill_pattern_t> pattern;
        core::item_view<sprite_draw_t>  sprite;
        core::item_view<draw_shape_t>   draw_shape;
        core::item_view<draw_shape_t>   cut_shape;
        core::item_view<scissor_rect_t> scissor;
        int32                           z_order;
    };

    using ui_render_data_types = core::type_list<
        color_stop_t,
        linear_gradient_t,
        radial_gradient_t,
        conic_gradient_t,
        paint_brush_t,
        fill_pattern_t,
        sprite_draw_t,
        quad_draw_t,
        line_draw_t,
        superellipse_draw_t,
        rect_draw_t,
        sdf_draw_t,
        draw_shape_t,
        scissor_rect_t,
        ui_draw_command_t>;

    using ui_render_data_t = core::basic_typed_vectors<ui_render_data_types>;


    class ui_renderer;

    class ui_draw_builder
    {
    public:
        ~ui_draw_builder() noexcept;

        [[nodiscard]] ui_draw_builder& fill_color(math::color32 color) noexcept;
        [[nodiscard]] ui_draw_builder& fill_linear(core::buffer_view<const color_stop_t> gradient, math::vec2 from, math::vec2 to, gradient_repeat_type repeat = gradient_repeat_type::clamp) noexcept;
        [[nodiscard]] ui_draw_builder& fill_radial(core::buffer_view<const color_stop_t> gradient, math::vec2 center, float radius, gradient_repeat_type repeat = gradient_repeat_type::clamp) noexcept;
        [[nodiscard]] ui_draw_builder& fill_conic(core::buffer_view<const color_stop_t> gradient, math::vec2 center, math::angle start = math::angle(0.0f), math::angle sweep = math::angle(0.0f), gradient_repeat_type repeat = gradient_repeat_type::clamp) noexcept;

        [[nodiscard]] ui_draw_builder& pattern_fill(fill_pattern_type type, math::angle a, float size0, float size1 = 0.0f) noexcept;

        [[nodiscard]] ui_draw_builder& shadow(float bias, float radius) noexcept;
        [[nodiscard]] ui_draw_builder& cut_shadow(float bias, float radius) noexcept;

        [[nodiscard]] ui_draw_builder& z_order(int32 z) noexcept;

        [[nodiscard]] ui_draw_builder& cut_quad(math::vec2 center, math::vec2 half_x, math::vec2 half_y) noexcept;
        [[nodiscard]] ui_draw_builder& cut_quad(geometry::aabb2 aabb) noexcept;
        [[nodiscard]] ui_draw_builder& cut_quad(geometry::obb2 obb) noexcept;

        [[nodiscard]] ui_draw_builder& cut_line(math::vec2 from, math::vec2 to, float thickness = 0.0f, float dash = 0.0f, float gap = 0.0f) noexcept;

        [[nodiscard]] ui_draw_builder& cut_superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash) noexcept;
        [[nodiscard]] ui_draw_builder& cut_superellipse(geometry::aabb2 aabb, float squash) noexcept;
        [[nodiscard]] ui_draw_builder& cut_superellipse(geometry::obb2 obb, float squash) noexcept;

        [[nodiscard]] ui_draw_builder& cut_rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius = {}) noexcept;
        [[nodiscard]] ui_draw_builder& cut_rect(geometry::aabb2 aabb, geometry::radius4 radius = {}) noexcept;
        [[nodiscard]] ui_draw_builder& cut_rect(geometry::obb2 obb, geometry::radius4 radius = {}) noexcept;

        [[nodiscard]] ui_draw_builder& cut_sdf(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0) noexcept;
        [[nodiscard]] ui_draw_builder& cut_sdf(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0) noexcept;
        [[nodiscard]] ui_draw_builder& cut_sdf(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0) noexcept;

        [[nodiscard]] ui_draw_builder& sprite(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept;
        [[nodiscard]] ui_draw_builder& sprite(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept;
        [[nodiscard]] ui_draw_builder& sprite(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept;

        void draw();

    private:
        friend class ui_renderer;

        ui_draw_builder(ui_renderer* r) noexcept;

    private:
        ui_renderer*      m_r = nullptr;
        ui_draw_command_t m_cur_cmd;

        debug::debug_only<bool> m_drawn = false;
    };


    class ui_renderer final : core::noncopyable
    {
    public:
        ui_renderer() = default;
        ~ui_renderer() noexcept = default;

        [[nodiscard]] ui_draw_builder quad(math::vec2 center, math::vec2 half_x, math::vec2 half_y) noexcept;
        [[nodiscard]] ui_draw_builder quad(geometry::aabb2 aabb) noexcept;
        [[nodiscard]] ui_draw_builder quad(geometry::obb2 obb) noexcept;

        [[nodiscard]] ui_draw_builder line(math::vec2 from, math::vec2 to, float thickness, float dash = 0.0f, float gap = 0.0f) noexcept;

        [[nodiscard]] ui_draw_builder superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash) noexcept;
        [[nodiscard]] ui_draw_builder superellipse(geometry::aabb2 aabb, float squash) noexcept;
        [[nodiscard]] ui_draw_builder superellipse(geometry::obb2 obb, float squash) noexcept;

        [[nodiscard]] ui_draw_builder rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius) noexcept;
        [[nodiscard]] ui_draw_builder rect(geometry::aabb2 aabb, geometry::radius4 radius = {}) noexcept;
        [[nodiscard]] ui_draw_builder rect(geometry::obb2 obb, geometry::radius4 radius = {}) noexcept;

        [[nodiscard]] ui_draw_builder sdf(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0f) noexcept;
        [[nodiscard]] ui_draw_builder sdf(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0f) noexcept;
        [[nodiscard]] ui_draw_builder sdf(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0f) noexcept;

        //[[nodiscard]] ui_draw_builder text(const text_archetype& text, uint32 first, uint32 size, rhi::texture_handle atlas);

        void push_scissor_rect(geometry::aabb2 sc);

        void pop_scissor_rect() noexcept;

        [[nodiscard]] ui_render_data_t take() noexcept;

    private:
        friend class ui_draw_builder;

        ui_render_data_t m_ui_render_data;

        core::vector<core::item_view<scissor_rect_t>> m_scissor_stack;
    };

} // namespace tavros::renderer

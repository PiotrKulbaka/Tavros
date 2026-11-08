#include <tavros/renderer/renderer/ui_renderer.hpp>

namespace tavros::renderer
{

    ui_draw_builder::ui_draw_builder(ui_renderer* r) noexcept
        : m_r(r)
    {
    }

    ui_draw_builder::~ui_draw_builder() noexcept
    {
        TAV_ASSERT(m_drawn);
    }

    ui_draw_builder& ui_draw_builder::fill_color(math::color32 color) noexcept
    {
        TAV_ASSERT(!m_cur_cmd.brush.valid());

        m_cur_cmd.brush = m_r->m_ui_render_data.add(
            paint_brush_t{
                .type = brush_fill_type::solid,
                .color = color
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::fill_linear(core::buffer_view<const color_stop_t> gradient, math::vec2 from, math::vec2 to, gradient_repeat_type repeat) noexcept
    {
        TAV_ASSERT(gradient.size() > 0);

        if (gradient.size() == 1) {
            return fill_color(gradient[0].color);
        }

        TAV_ASSERT(!m_cur_cmd.brush.valid());

        auto color_range_view = m_r->m_ui_render_data.add_range(gradient);

        auto linear_view = m_r->m_ui_render_data.add(
            linear_gradient_t{
                .paint = color_range_view,
                .from = from,
                .to = to,
                .repeat = repeat
            }
        );

        m_cur_cmd.brush = m_r->m_ui_render_data.add(
            paint_brush_t{
                .type = brush_fill_type::linear,
                .linear_view = linear_view
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::fill_radial(core::buffer_view<const color_stop_t> gradient, math::vec2 center, float radius, gradient_repeat_type repeat) noexcept
    {
        TAV_ASSERT(gradient.size() > 0);

        if (gradient.size() == 1) {
            return fill_color(gradient[0].color);
        }

        TAV_ASSERT(!m_cur_cmd.brush.valid());

        auto color_range_view = m_r->m_ui_render_data.add_range(gradient);

        auto radial_view = m_r->m_ui_render_data.add(
            radial_gradient_t{
                .paint = color_range_view,
                .center = center,
                .radius = radius,
                .repeat = repeat
            }
        );

        m_cur_cmd.brush = m_r->m_ui_render_data.add(
            paint_brush_t{
                .type = brush_fill_type::radial,
                .linear_view = radial_view
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::fill_conic(core::buffer_view<const color_stop_t> gradient, math::vec2 center, math::angle start, math::angle sweep, gradient_repeat_type repeat) noexcept
    {
        TAV_ASSERT(gradient.size() > 0);

        if (gradient.size() == 1) {
            return fill_color(gradient[0].color);
        }

        TAV_ASSERT(!m_cur_cmd.brush.valid());

        auto color_range_view = m_r->m_ui_render_data.add_range(gradient);

        auto conic_view = m_r->m_ui_render_data.add(
            conic_gradient_t{
                .paint = color_range_view,
                .center = center,
                .start = start,
                .sweep = sweep,
                .repeat = repeat
            }
        );

        m_cur_cmd.brush = m_r->m_ui_render_data.add(
            paint_brush_t{
                .type = brush_fill_type::conic,
                .conic_view = conic_view
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::pattern_fill(fill_pattern_type type, math::angle a, float size0, float size1) noexcept
    {
        TAV_ASSERT(!m_cur_cmd.pattern.valid());

        m_cur_cmd.pattern = m_r->m_ui_render_data.add(
            fill_pattern_t{
                .type = type,
                .angle = a,
                .size0 = size0,
                .size1 = size1
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::shadow(float bias, float radius) noexcept
    {
        auto& cur_draw_shape = m_r->m_ui_render_data[m_cur_cmd.draw_shape];
        cur_draw_shape.shadow_bias = bias;
        cur_draw_shape.shadow_radius = radius;

        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_shadow(float bias, float radius) noexcept
    {
        auto& cur_cut_shape = m_r->m_ui_render_data[m_cur_cmd.cut_shape];
        cur_cut_shape.shadow_bias = bias;
        cur_cut_shape.shadow_radius = radius;

        return *this;
    }

    ui_draw_builder& ui_draw_builder::z_order(int32 z) noexcept
    {
        m_cur_cmd.z_order = z;

        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_quad(math::vec2 center, math::vec2 half_x, math::vec2 half_y) noexcept
    {
        TAV_ASSERT(!m_cur_cmd.cut_shape.valid());

        auto cut_quad_view = m_r->m_ui_render_data.add(
            quad_draw_t{
                .center = center,
                .half_x = half_x,
                .half_y = half_y
            }
        );

        m_cur_cmd.cut_shape = m_r->m_ui_render_data.add(
            draw_shape_t{
                .type = shape_type::quad,
                .sdf_view = cut_quad_view,
                .shadow_bias = 0.0f,
                .shadow_radius = 0.0f
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_quad(geometry::aabb2 aabb) noexcept
    {
        return cut_quad(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f));
    }

    ui_draw_builder& ui_draw_builder::cut_quad(geometry::obb2 obb) noexcept
    {
        return cut_quad(obb.center, obb.right, obb.up);
    }

    ui_draw_builder& ui_draw_builder::cut_line(math::vec2 from, math::vec2 to, float thickness, float dash, float gap) noexcept
    {
        TAV_ASSERT(!m_cur_cmd.cut_shape.valid());

        auto cut_line_view = m_r->m_ui_render_data.add(
            line_draw_t{
                .from = from,
                .to = to,
                .thickness = thickness,
                .dash = dash,
                .gap = gap
            }
        );

        m_cur_cmd.cut_shape = m_r->m_ui_render_data.add(
            draw_shape_t{
                .type = shape_type::line,
                .sdf_view = cut_line_view,
                .shadow_bias = 0.0f,
                .shadow_radius = 0.0f
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash) noexcept
    {
        TAV_ASSERT(!m_cur_cmd.cut_shape.valid());

        auto cut_superellipse_view = m_r->m_ui_render_data.add(
            superellipse_draw_t{
                .center = center,
                .half_x = half_x,
                .half_y = half_y,
                .squash = squash
            }
        );

        m_cur_cmd.cut_shape = m_r->m_ui_render_data.add(
            draw_shape_t{
                .type = shape_type::superellipse,
                .sdf_view = cut_superellipse_view,
                .shadow_bias = 0.0f,
                .shadow_radius = 0.0f
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_superellipse(geometry::aabb2 aabb, float squash) noexcept
    {
        return cut_superellipse(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), squash);
    }

    ui_draw_builder& ui_draw_builder::cut_superellipse(geometry::obb2 obb, float squash) noexcept
    {
        return cut_superellipse(obb.center, obb.right, obb.up, squash);
    }

    ui_draw_builder& ui_draw_builder::cut_rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius) noexcept
    {
        TAV_ASSERT(!m_cur_cmd.cut_shape.valid());

        auto cut_rect_view = m_r->m_ui_render_data.add(
            rect_draw_t{
                .center = center,
                .half_x = half_x,
                .half_y = half_y,
                .radius = radius
            }
        );

        m_cur_cmd.cut_shape = m_r->m_ui_render_data.add(
            draw_shape_t{
                .type = shape_type::rect,
                .sdf_view = cut_rect_view,
                .shadow_bias = 0.0f,
                .shadow_radius = 0.0f
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_rect(geometry::aabb2 aabb, geometry::radius4 radius) noexcept
    {
        return cut_rect(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), radius);
    }

    ui_draw_builder& ui_draw_builder::cut_rect(geometry::obb2 obb, geometry::radius4 radius) noexcept
    {
        return cut_rect(obb.center, obb.right, obb.up, radius);
    }

    ui_draw_builder& ui_draw_builder::cut_sdf(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        TAV_ASSERT(!m_cur_cmd.cut_shape.valid());

        auto cut_sdf_view = m_r->m_ui_render_data.add(
            sdf_draw_t{
                .center = center,
                .half_x = half_x,
                .half_y = half_y,
                .atlas_rect = atlas_rect,
                .tex = tex,
                .sdf_pad_size = sdf_pad_size
            }
        );

        m_cur_cmd.cut_shape = m_r->m_ui_render_data.add(
            draw_shape_t{
                .type = shape_type::sdf,
                .sdf_view = cut_sdf_view,
                .shadow_bias = 0.0f,
                .shadow_radius = 0.0f
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_sdf(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        return cut_sdf(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), tex, atlas_rect, sdf_pad_size);
    }

    ui_draw_builder& ui_draw_builder::cut_sdf(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        return cut_sdf(obb.center, obb.right, obb.up, tex, atlas_rect, sdf_pad_size);
    }

    ui_draw_builder& ui_draw_builder::sprite(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept
    {
        TAV_ASSERT(!m_cur_cmd.sprite.valid());

        m_cur_cmd.sprite = m_r->m_ui_render_data.add(
            sprite_draw_t{
                .center = center,
                .half_x = half_x,
                .half_y = half_y,
                .atlas_rect = sprite_rect,
                .tex = tex
            }
        );

        return *this;
    }

    ui_draw_builder& ui_draw_builder::sprite(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept
    {
        return sprite(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), tex, sprite_rect);
    }

    ui_draw_builder& ui_draw_builder::sprite(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept
    {
        return sprite(obb.center, obb.right, obb.up, tex, sprite_rect);
    }

    void ui_draw_builder::draw()
    {
        TAV_ASSERT(!m_drawn);
        TAV_DEBUG_ONLY(m_drawn = true);

        if (!m_r->m_scissor_stack.empty()) {
            m_cur_cmd.scissor = m_r->m_scissor_stack.back();
        }

        m_r->m_ui_render_data.add(m_cur_cmd);
    }


    //================================================================
    //                          UI RENDERER
    //================================================================

    ui_draw_builder ui_renderer::quad(math::vec2 center, math::vec2 half_x, math::vec2 half_y) noexcept
    {
        auto quad_draw_view = m_ui_render_data.add(
            quad_draw_t{
                .center = center,
                .half_x = half_x,
                .half_y = half_y
            }
        );

        auto draw_shape_view = m_ui_render_data.add(
            draw_shape_t{
                .type = shape_type::line,
                .quad_view = quad_draw_view,
                .shadow_bias = 0.0f,
                .shadow_radius = 0.0f
            }
        );

        ui_draw_builder builder(this);
        builder.m_cur_cmd.draw_shape = draw_shape_view;

        return builder;
    }

    ui_draw_builder ui_renderer::quad(geometry::aabb2 aabb) noexcept
    {
        return quad(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f));
    }

    ui_draw_builder ui_renderer::quad(geometry::obb2 obb) noexcept
    {
        return quad(obb.center, obb.right, obb.up);
    }

    ui_draw_builder ui_renderer::line(math::vec2 from, math::vec2 to, float thickness, float dash, float gap) noexcept
    {
        auto line_draw_view = m_ui_render_data.add(
            line_draw_t{
                .from = from,
                .to = to,
                .thickness = thickness,
                .dash = dash,
                .gap = gap
            }
        );

        auto draw_shape_view = m_ui_render_data.add(
            draw_shape_t{
                .type = shape_type::line,
                .line_view = line_draw_view,
                .shadow_bias = 0.0f,
                .shadow_radius = 0.0f
            }
        );

        ui_draw_builder builder(this);
        builder.m_cur_cmd.draw_shape = draw_shape_view;

        return builder;
    }

    ui_draw_builder ui_renderer::superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash) noexcept
    {
        auto superellipse_draw_view = m_ui_render_data.add(
            superellipse_draw_t{
                .center = center,
                .half_x = half_x,
                .half_y = half_y,
                .squash = squash
            }
        );

        auto draw_shape_view = m_ui_render_data.add(
            draw_shape_t{
                .type = shape_type::superellipse,
                .superellipse_view = superellipse_draw_view,
                .shadow_bias = 0.0f,
                .shadow_radius = 0.0f
            }
        );

        ui_draw_builder builder(this);
        builder.m_cur_cmd.draw_shape = draw_shape_view;

        return builder;
    }

    ui_draw_builder ui_renderer::superellipse(geometry::aabb2 aabb, float squash) noexcept
    {
        return superellipse(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), squash);
    }

    ui_draw_builder ui_renderer::superellipse(geometry::obb2 obb, float squash) noexcept
    {
        return superellipse(obb.center, obb.right, obb.up, squash);
    }

    ui_draw_builder ui_renderer::rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius) noexcept
    {
        auto rect_draw_view = m_ui_render_data.add(
            rect_draw_t{
                .center = center,
                .half_x = half_x,
                .half_y = half_y,
                .radius = radius
            }
        );

        auto draw_shape_view = m_ui_render_data.add(
            draw_shape_t{
                .type = shape_type::rect,
                .rect_view = rect_draw_view,
                .shadow_bias = 0.0f,
                .shadow_radius = 0.0f
            }
        );

        ui_draw_builder builder(this);
        builder.m_cur_cmd.draw_shape = draw_shape_view;

        return builder;
    }

    ui_draw_builder ui_renderer::rect(geometry::aabb2 aabb, geometry::radius4 radius) noexcept
    {
        return rect(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), radius);
    }

    ui_draw_builder ui_renderer::rect(geometry::obb2 obb, geometry::radius4 radius) noexcept
    {
        return rect(obb.center, obb.right, obb.up, radius);
    }

    ui_draw_builder ui_renderer::sdf(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle atlas, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        auto sdf_draw_view = m_ui_render_data.add(
            sdf_draw_t{
                .center = center,
                .half_x = half_x,
                .half_y = half_y,
                .atlas_rect = atlas_rect,
                .tex = atlas,
                .sdf_pad_size = sdf_pad_size
            }
        );

        auto draw_shape_view = m_ui_render_data.add(
            draw_shape_t{
                .type = shape_type::sdf,
                .sdf_view = sdf_draw_view,
                .shadow_bias = 0.0f,
                .shadow_radius = 0.0f
            }
        );

        ui_draw_builder builder(this);
        builder.m_cur_cmd.draw_shape = draw_shape_view;

        return builder;
    }

    ui_draw_builder ui_renderer::sdf(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        return sdf(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), tex, atlas_rect, sdf_pad_size);
    }

    ui_draw_builder ui_renderer::sdf(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        return sdf(obb.center, obb.right, obb.up, tex, atlas_rect, sdf_pad_size);
    }

    void ui_renderer::push_scissor_rect(geometry::aabb2 sc)
    {
        auto v = m_ui_render_data.add(
            scissor_rect_t{
                sc.left,
                sc.top,
                sc.right,
                sc.bottom
            }
        );
        m_scissor_stack.push_back(v);
    }

    void ui_renderer::pop_scissor_rect() noexcept
    {
        TAV_ASSERT(!m_scissor_stack.empty());
        if (!m_scissor_stack.empty()) {
            m_scissor_stack.pop_back();
        }
    }

    ui_render_data_t ui_renderer::take() noexcept
    {
        TAV_ASSERT(m_scissor_stack.empty());
        m_scissor_stack.clear();
        return std::move(m_ui_render_data);
    }

} // namespace tavros::renderer

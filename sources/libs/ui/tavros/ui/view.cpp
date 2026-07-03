#include <tavros/ui/view.hpp>

#include <tavros/core/debug/unreachable.hpp>

namespace tavros::ui
{

    view::view() noexcept
        : m_is_visible(true)
        , m_is_enabled(true)
        , m_size(0.0f, 0.0f)
        , m_min_size(0.0f, 0.0f)
        , m_max_size(std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
        , m_padding(0.0f, 0.0f, 0.0f, 0.0f)
        , m_margin(0.0f, 0.0f, 0.0f, 0.0f)
        , m_bounds(0.0f, 0.0f, 0.0f, 0.0f)
    {
    }

    view::~view() noexcept
    {
    }

    void view::set_visible(bool v) noexcept
    {
        m_is_visible = v;
    }

    void view::set_enabled(bool e) noexcept
    {
        m_is_enabled = e;
    }

    bool view::is_visible() const noexcept
    {
        return m_is_visible;
    }

    bool view::is_enabled() const noexcept
    {
        return m_is_enabled;
    }


    void view::set_size(math::vec2 size) noexcept
    {
        m_size = size;
    }

    void view::set_min_size(math::vec2 min_size) noexcept
    {
        m_min_size = min_size;
    }

    void view::set_max_size(math::vec2 max_size) noexcept
    {
        m_max_size = max_size;
    }

    math::vec2 view::size() const noexcept
    {
        return m_size;
    }

    math::vec2 view::min_size() const noexcept
    {
        return m_min_size;
    }

    math::vec2 view::max_size() const noexcept
    {
        return m_max_size;
    }

    void view::set_padding(geometry::thickness4 pad) noexcept
    {
        m_padding = pad;
    }

    void view::set_margin(geometry::thickness4 margin) noexcept
    {
        m_margin = margin;
    }

    geometry::thickness4 view::padding() const noexcept
    {
        return m_padding;
    }

    geometry::thickness4 view::margin() const noexcept
    {
        return m_margin;
    }

    math::vec2 view::measure(math::vec2) const noexcept
    {
        return limit_content_size(m_size);
    }

    void view::arrange(geometry::aabb2 bounds) noexcept
    {
        auto sz = limit_content_size(bounds.size());
        m_bounds = {bounds.min, bounds.min + sz};
    }

    geometry::aabb2 view::margin_bounds() const noexcept
    {
        return {
            m_bounds.left - m_margin.left,
            m_bounds.top - m_margin.top,
            m_bounds.right + m_margin.right,
            m_bounds.bottom + m_margin.bottom
        };
    }

    geometry::aabb2 view::bounds() const noexcept
    {
        return m_bounds;
    }

    geometry::aabb2 view::content_bounds() const noexcept
    {
        return {
            m_bounds.left + m_padding.left,
            m_bounds.top + m_padding.top,
            m_bounds.right - m_padding.right,
            m_bounds.bottom - m_padding.bottom
        };
    }

    void view::update(float dt)
    {
        if (!m_is_visible) {
            return;
        }
        for (auto& child : children()) {
            child.update(dt);
        }
    }

    void view::draw(const render_context& rctx)
    {
        if (!m_is_visible) {
            return;
        }

        if (rctx.debug_draw) {
            auto* r = rctx.renderer->renderer2d();
            r->set_line_thickness(3.0f);

            if (m_is_hover) {
                // Content bounds
                r->set_brush_solid_color(0x7F92FF60);
                auto cb = content_bounds();
                r->fill_rect(cb);

                // Size bounds
                r->set_brush_solid_color(0x7FFFC560);
                auto sb = bounds();
                r->fill_rect_cut(sb, cb);

                // Margin bounds
                r->set_brush_solid_color(0xFFE97F60);
                auto mb = margin_bounds();
                r->fill_rect_cut(mb, sb);
            }

            // Bounds
            r->set_brush_solid_color(0x7FFF8EFF);
            rctx.renderer->renderer2d()->draw_aabb(bounds());

            // if (m_is_hover) {
            //     r->set_brush_solid_color(0x0094FFFF);

            //    // Content bounds
            //    r->set_brush_solid_color(0x0094FFFF);
            //    rctx.renderer->renderer2d()->draw_aabb(content_bounds());
            //}
        }

        for (auto& child : children()) {
            child.draw(rctx);
        }
    }

    bool view::hit_test(math::vec2 pos) const noexcept
    {
        return m_is_visible && m_bounds.contains_point(pos);
    }

    void view::on_mouse_down(const mouse_button_event_args&)
    {
    }

    void view::on_mouse_move(const mouse_move_event_args&)
    {
    }

    void view::on_mouse_up(const mouse_button_event_args&)
    {
    }

    void view::on_mouse_wheel(const mouse_wheel_event_args&)
    {
    }

    void view::on_mouse_hover()
    {
        m_is_hover = true;
    }

    void view::on_mouse_leave()
    {
        m_is_hover = false;
    }

    void view::on_key_down(const keyboard_key_event_args&)
    {
    }

    void view::on_key_up(const keyboard_key_event_args&)
    {
    }

    void view::on_key_press(const key_press_event_args&)
    {
    }

    math::vec2 view::limit_content_size(math::vec2 size) const noexcept
    {
        auto min_sz = math::max(m_padding.size(), m_min_size);
        auto max_sz = math::max(min_sz, m_max_size);
        return math::clamp(size, min_sz, max_sz);
    }

} // namespace tavros::ui

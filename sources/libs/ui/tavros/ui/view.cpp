#include <tavros/ui/view.hpp>

#include <tavros/core/debug/unreachable.hpp>

namespace tavros::ui
{

    view::view()
    {
        m_pos = {0.0f, 0.0f};
        m_size = {0.0f, 0.0f};
        m_pad = {0.0f, 0.0f, 0.0f, 0.0f};

        m_is_hovered = false;
        m_is_clicked = false;
        m_hover_factor = 0.0f;
        m_click_factor = 0.0f;

        m_is_enabled = true;
    }

    rect2 view::padding_rect() const
    {
        auto p = absolute_position();
        auto outer_sz = outer_size();
        return {p, p + outer_sz};
    }

    rect2 view::content_rect() const
    {
        auto p = absolute_content_position();
        auto sz = size();
        return {p, p + sz};
    }

    void view::set_padding(padding2 pad)
    {
        m_pad = pad;
    }

    padding2 view::padding() const
    {
        return m_pad;
    }

    void view::set_position(const point2 pos)
    {
        m_pos = pos;
    }

    point2 view::position() const
    {
        return m_pos;
    }

    point2 view::absolute_position() const
    {
        return absolute_content_position() - m_pad.min;
    }

    point2 view::absolute_content_position() const
    {
        if (auto p = parent()) {
            return p->absolute_content_position() + m_pos + m_pad.min;
        }
        return m_pos + m_pad.min;
    }

    void view::set_size(const size2 size)
    {
        m_size = size;

        for (auto& ch : children()) {
            ch.on_parent_resized(size);
        }
    }

    size2 view::size() const
    {
        return m_size;
    }

    size2 view::outer_size() const
    {
        return {m_size.width + m_pad.left + m_pad.right, m_size.height + m_pad.top + m_pad.bottom};
    }

    void view::set_enabled(bool enabled)
    {
        m_is_enabled = enabled;
    }

    bool view::is_enabled() const
    {
        return m_is_enabled;
    }

    void view::update(float dt)
    {
        m_hover_factor = math::clamp(m_hover_factor + (m_is_hovered ? dt : -dt) * 5.0f, 0.0f, 1.0f);
        m_click_factor = math::clamp(m_click_factor + (m_is_clicked ? dt : -dt) * 15.0f, 0.0f, 1.0f);
    }

    void view::draw(const render_context& rctx)
    {
        /*if (is_subtree_enabled()) {
            math::color cl_inner = {1.0f, 0.5f, 0.5f, 0.25};
            math::color cl1 = {0.6f, 0.6f, 0.8f, 0.15};
            math::color cl2 = {1.0f, 0.3f, 0.7f, 0.47};
            math::color cl3 = {1.0f, 1.0f, 1.0f, 0.55};
            math::color cl = math::lerp(cl1, cl2, m_hover_factor);
            cl = math::lerp(cl, cl3, m_click_factor);

            auto cr = content_rect();
            cr.min -= vec2(3.0f) * m_hover_factor;
            cr.max += vec2(3.0f) * m_hover_factor;

            cr.min += vec2(0.0f, 3.0f) * m_click_factor;
            cr.max += vec2(0.0f, 3.0f) * m_click_factor;

            rctx.drenderer->box2d(cr, cl, renderer::debug_renderer::draw_mode::faces);
            rctx.drenderer->box2d(padding_rect(), cl_inner, renderer::debug_renderer::draw_mode::edges);
        } else {
            math::color cl_inner = {0.7f, 0.7f, 0.7f, 0.25};
            math::color cl_inner_h = {0.7f, 0.7f, 0.7f, 0.55};
            math::color cl_outer = {0.7f, 0.7f, 0.7f, 0.47};
            auto        cl = math::lerp(cl_inner, cl_inner_h, m_hover_factor);


            rctx.drenderer->box2d(content_rect(), cl, renderer::debug_renderer::draw_mode::faces);
            rctx.drenderer->box2d(padding_rect(), cl_outer, renderer::debug_renderer::draw_mode::edges);
        }*/
    }

    void view::on_mouse_down(const mouse_button_event_args& e)
    {
        if (e.button == input::mouse_button::left) {
            m_is_clicked = true;
        }
    }

    void view::on_mouse_move(const mouse_move_event_args& e)
    {
    }

    void view::on_mouse_up(const mouse_button_event_args& e)
    {
        if (e.button == input::mouse_button::left) {
            m_is_clicked = false;
        }
    }

    void view::on_mouse_wheel(const mouse_wheel_event_args& e)
    {
    }

    void view::on_mouse_hover()
    {
        m_is_hovered = true;
    }

    void view::on_mouse_leave()
    {
        m_is_hovered = false;
        m_is_clicked = false;
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

    void view::on_parent_resized(const size2 parent_size)
    {
    }

    bool view::test_hit(point2 p) const noexcept
    {
        return content_rect().contains_point(p);
    }

    bool view::is_subtree_enabled() const
    {
        if (m_is_enabled) {
            if (auto p = parent()) {
                return p->is_subtree_enabled();
            }
            return true;
        }
        return false;
    }

} // namespace tavros::ui

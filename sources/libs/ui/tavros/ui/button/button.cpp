#include <tavros/ui/button/button.hpp>

#include <tavros/core/logger/logger.hpp>

namespace tavros::ui
{

    button::button()
        : m_is_pressed(false)
        , m_is_hovered(false)
        , m_hover_time(0.0f)
    {
    }

    button::~button()
    {
    }

    void button::set_text(core::string_view text)
    {
        m_text = text;
    }

    void button::update(float dt)
    {
        m_hover_factor = math::clamp(m_hover_factor + (m_is_hovered ? dt : -dt) * 5.0f, 0.0f, 1.0f);
        m_click_factor = math::clamp(m_click_factor + (m_is_pressed ? dt : -dt) * 15.0f, 0.0f, 1.0f);

        if (m_is_hovered) {
            m_hover_time += dt;
        } else {
            m_hover_time = 0;
        }
    }

    void button::draw(const render_context& rctx)
    {
        math::color clicked_color = {1.0f, 0.0f, 0.0f, 0.25f};
        math::color hovered_color = {0.0f, 1.0f, 0.0f, 0.25f};
        math::color normal_color = {0.0f, 0.0f, 1.0f, 0.25f};
        math::color disabled_color = {1.0f, 1.0f, 1.0f, 0.25f};
        math::color disabled_hovered_color = {0.5f, 0.5f, 0.5f, 0.25f};

        auto cr = content_rect();

        if (is_subtree_enabled()) {
            auto cl = math::lerp(normal_color, hovered_color, m_hover_factor);

            auto text_h = cr.size().height;

            if (m_is_hovered) {
                auto factor = std::fmod(m_hover_time, 1.0f);
                if (factor > 0.5f) {
                    factor = 1.0f - factor;
                }
                factor *= 2.0f;

                math::color blinking_color = {0.5f, 0.5f, 0.5f, 0.25f};
                cl = math::lerp(cl, blinking_color, factor);

                text_h = math::lerp(text_h, text_h + factor * 6.0f, factor);
            }

            cl = math::lerp(cl, clicked_color, m_click_factor);

            cr.min += vec2(0.0f, 3.0f) * m_click_factor;
            cr.max += vec2(0.0f, 3.0f) * m_click_factor;


            rctx.drenderer->box2d(cr, cl, renderer::debug_renderer::draw_mode::faces);
            rctx.drenderer->draw_text2d(m_text, text_h / 2, {text::text_align::center, renderer::debug_renderer::vertical_align::center, 1.0f}, cr, {1, 1, 1, 1});
        } else {
            auto cl = math::lerp(disabled_color, disabled_hovered_color, m_hover_factor);
            rctx.drenderer->box2d(cr, cl, renderer::debug_renderer::draw_mode::faces);

            auto text_h = cr.size().height;
            rctx.drenderer->draw_text2d(m_text, text_h / 2, {text::text_align::center, renderer::debug_renderer::vertical_align::center, 1.0f}, cr, {1, 1, 1, 1});
        }
    }

    void button::on_click()
    {
        core::logger::print(core::severity_level::info, "button", "clicked");
    }

    void button::on_mouse_down(const mouse_button_event_args& e)
    {
        if (e.button == input::mouse_button::left) {
            if (is_subtree_enabled()) {
                m_is_pressed = true;
            }
        }
    }

    void button::on_mouse_up(const mouse_button_event_args& e)
    {
        if (e.button == input::mouse_button::left) {
            if (m_is_pressed && is_subtree_enabled()) {
                on_click();
            }
            m_is_pressed = false;
        }
    }

    void button::on_mouse_hover()
    {
        m_is_hovered = true;
    }

    void button::on_mouse_leave()
    {
        m_is_hovered = false;
        m_is_pressed = false;
    }

} // namespace tavros::ui
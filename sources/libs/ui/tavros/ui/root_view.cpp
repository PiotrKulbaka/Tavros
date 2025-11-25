#include <tavros/ui/root_view.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/unreachable.hpp>

namespace
{
    tavros::core::logger logger("root_view");

    template<class Func>
    void traverse_preorder(tavros::ui::view* node, Func&& func)
    {
        if (!node) {
            return;
        }

        func(node);
        for (auto child = node->first_child(); child; child = child->next()) {
            traverse_preorder(child, std::forward<Func>(func));
        }
    }

    template<class Test, class Func>
    void traverse_preorder(tavros::ui::view* node, Test&& test, Func&& func)
    {
        if (!node) {
            return;
        }

        if (!test(node)) {
            return;
        }

        func(node);
        for (auto child = node->first_child(); child; child = child->next()) {
            traverse_preorder(child, std::forward<Test>(test), std::forward<Func>(func));
        }
    }

    template<class Test, class Func>
    void traverse_postorder(tavros::ui::view* node, Test&& test, Func&& func)
    {
        if (!node) {
            return;
        }

        if (!test(node)) {
            return;
        }

        for (auto child = node->first_child(); child; child = child->next()) {
            traverse_postorder(child, std::forward<Test>(test), std::forward<Func>(func));
        }

        func(node);
    }

} // namespace

namespace tavros::ui
{

    root_view::root_view()
        : m_graphics_device(nullptr)
        , m_is_active(true)
        , m_keyboard_focus(nullptr)
        , m_hovered(nullptr)
    {
    }

    root_view::~root_view()
    {
    }

    void root_view::init(rhi::graphics_device* gdevice)
    {
        TAV_ASSERT(gdevice);
        m_graphics_device = gdevice;
        m_debug_renderer.init(gdevice);
    }

    void root_view::shutdown()
    {
        m_debug_renderer.shutdown();
    }

    void root_view::process_ui_events(input::event_args_queue_view events)
    {
        for (auto& e : events) {
            if (!m_is_active) {
                if (input::event_type::activate == e.type) {
                    m_is_active = true;
                }
                continue;
            }

            switch (e.type) {
            case input::event_type::none:
                ::logger.warning("event `none` received");
                break;

            case input::event_type::mouse_down: {
                mouse_button_event_args a;
                a.button = e.button;
                a.pos = e.vec;
                a.time_us = e.time_us;
                on_mouse_down(a);
            } break;

            case input::event_type::mouse_move: {
                mouse_move_event_args a;
                a.pos = e.vec;
                a.time_us = e.time_us;
                on_mouse_move(a);
            } break;

            case input::event_type::mouse_move_delta:
                // Skip this
                break;

            case input::event_type::mouse_up: {
                mouse_button_event_args a;
                a.button = e.button;
                a.pos = e.vec;
                a.time_us = e.time_us;
                on_mouse_up(a);
            } break;

            case input::event_type::mouse_wheel: {
                mouse_wheel_event_args a;
                a.pos = e.vec;
                a.wheel_delta = e.wheel;
                a.time_us = e.time_us;
                on_mouse_wheel(a);
            } break;

            case input::event_type::key_down: {
                keyboard_key_event_args a;
                a.key = e.key;
                a.time_us = e.time_us;
                on_key_down(a);
            } break;

            case input::event_type::key_press: {
                key_press_event_args a;
                a.key_char = e.key_char;
                a.repeats = e.repeats;
                a.time_us = e.time_us;
                on_key_press(a);
            } break;

            case input::event_type::key_up: {
                keyboard_key_event_args a;
                a.key = e.key;
                a.time_us = e.time_us;
                on_key_up(a);
            } break;

            case input::event_type::window_size:
                m_root.set_size(e.vec);
                m_orto_proj = math::mat4::ortho(0.0f, e.vec.x, e.vec.y, 0.0f, 1.0f, -1.0f);
                break;

            case input::event_type::window_move:
                // Skip this
                break;

            case input::event_type::activate:
                m_is_active = true;
                break;

            case input::event_type::deactivate:
                m_is_active = false;
                break;

            default:
                TAV_UNREACHABLE();
                break;
            }
        }
    }

    void root_view::on_frame(rhi::command_queue* cmds, float delta_time)
    {
        layout();
        update(delta_time);
        render(cmds);
    }

    void root_view::update(float delta_time)
    {
        traverse_preorder(&m_root, [delta_time](auto* n) { n->update(delta_time); });
    }

    void root_view::layout()
    {
    }

    void root_view::render(rhi::command_queue* cmds)
    {
        render_context rctx;
        rctx.drenderer = &m_debug_renderer;
        rctx.gdevice = m_graphics_device;
        m_debug_renderer.begin_frame(m_orto_proj, math::mat4::identity());

        traverse_preorder(&m_root, [&](auto* n) { n->draw(rctx); });

        m_debug_renderer.update();
        m_debug_renderer.render(cmds);
        m_debug_renderer.end_frame();
    }

    void root_view::on_mouse_down(const mouse_button_event_args& e)
    {
        view* v = nullptr;
        traverse_preorder(&m_root, [&](auto* n) { return n->test_hit(e.pos); }, [&](auto* n) { v = n; });
        if (v) {
            v->on_mouse_down(e);
        }
    }

    void root_view::on_mouse_move(const mouse_move_event_args& e)
    {
        view* v = nullptr;
        traverse_preorder(&m_root, [&](auto* n) { return n->test_hit(e.pos); }, [&](auto* n) { v = n; });
        if (m_hovered != v) {
            if (m_hovered) {
                m_hovered->on_mouse_leave();
            }
            m_hovered = v;
            if (m_hovered) {
                m_hovered->on_mouse_hover();
            }
        }

        if (v) {
            v->on_mouse_move(e);
        }
    }

    void root_view::on_mouse_up(const mouse_button_event_args& e)
    {
        view* v = nullptr;
        traverse_preorder(&m_root, [&](auto* n) { return n->test_hit(e.pos); }, [&](auto* n) { v = n; });
        if (v) {
            v->on_mouse_up(e);
        }
    }

    void root_view::on_mouse_wheel(const mouse_wheel_event_args& e)
    {
        if (m_hovered) {
            m_hovered->on_mouse_wheel(e);
        }
    }

    void root_view::on_key_down(const keyboard_key_event_args& e)
    {
        if (m_keyboard_focus) {
            m_keyboard_focus->on_key_down(e);
        }
    }

    void root_view::on_key_up(const keyboard_key_event_args& e)
    {
        if (m_keyboard_focus) {
            m_keyboard_focus->on_key_up(e);
        }
    }

    void root_view::on_key_press(const key_press_event_args& e)
    {
        if (m_keyboard_focus) {
            m_keyboard_focus->on_key_press(e);
        }
    }

} // namespace tavros::ui

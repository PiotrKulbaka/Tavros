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

    root_view::root_view(core::shared_ptr<renderer::render_system> rs)
        : m_rs(std::move(rs))
    {
    }

    root_view::~root_view() noexcept
    {
    }

    void root_view::resolve()
    {
        if (!m_root) {
            return;
        }

        ui_context ctx(m_rs);

        traverse_preorder(m_root, [&](auto* n) { return n->is_enabled(); }, [&](auto* n) { n->resolve(ctx); });
    }

    void root_view::process_input(input::event_args_queue_view events) noexcept
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

            case input::event_type::mouse_enter:
                break;

            case input::event_type::mouse_leave:
                if (m_hovered) {
                    m_hovered->on_mouse_leave();
                }
                m_hovered = nullptr;
                break;

            case input::event_type::mouse_down: {
                mouse_button_event_args a;
                a.button = e.button;
                a.pos = e.vec;
                a.time_us = e.time_us;
                mouse_down(a);
            } break;

            case input::event_type::mouse_move: {
                mouse_move_event_args a;
                a.pos = e.vec;
                a.time_us = e.time_us;
                mouse_move(a);
            } break;

            case input::event_type::mouse_move_delta:
                // Skip this
                break;

            case input::event_type::mouse_up: {
                mouse_button_event_args a;
                a.button = e.button;
                a.pos = e.vec;
                a.time_us = e.time_us;
                mouse_up(a);
            } break;

            case input::event_type::mouse_wheel: {
                mouse_wheel_event_args a;
                a.pos = e.vec;
                a.wheel_delta = e.wheel;
                a.time_us = e.time_us;
                if (m_hovered) {
                    m_hovered->on_mouse_wheel(a);
                }
            } break;

            case input::event_type::key_down: {
                keyboard_key_event_args a;
                a.key = e.key;
                a.time_us = e.time_us;
                if (m_keyboard_focus) {
                    m_keyboard_focus->on_key_down(a);
                }
            } break;

            case input::event_type::key_press: {
                key_press_event_args a;
                a.key_char = e.key_char;
                a.repeats = e.repeats;
                a.time_us = e.time_us;
                if (m_keyboard_focus) {
                    m_keyboard_focus->on_key_press(a);
                }
            } break;

            case input::event_type::key_up: {
                keyboard_key_event_args a;
                a.key = e.key;
                a.time_us = e.time_us;
                if (m_keyboard_focus) {
                    m_keyboard_focus->on_key_up(a);
                }
            } break;

            case input::event_type::window_size:
                break;

            case input::event_type::window_move:
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

    void root_view::layout(geometry::aabb2 bounds) noexcept
    {
        if (!m_root) {
            return;
        }
        m_root->set_size(bounds.size());
        m_root->set_max_size(bounds.size());
        m_root->arrange(bounds);
    }

    void root_view::update(float dt) noexcept
    {
        if (!m_root) {
            return;
        }
        m_root->update(dt);
    }

    void root_view::draw()
    {
        if (!m_root) {
            return;
        }
        ui_context ctx(m_rs);
        m_root->draw(ctx);
        m_rs->add_data(renderer::fd_ui_draw_data{std::move(ctx.ui_renderer().take())});
    }

    void root_view::mouse_down(const mouse_button_event_args& e) noexcept
    {
        if (!m_root) {
            return;
        }

        view* v = nullptr;
        traverse_preorder(m_root, [&](auto* n) { return n->is_visible() && n->hit_test(e.pos); }, [&](auto* n) { v = n; });
        if (v) {
            v->on_mouse_down(e);
        }
    }

    void root_view::mouse_move(const mouse_move_event_args& e) noexcept
    {
        if (!m_root) {
            return;
        }

        view* v = nullptr;
        traverse_preorder(m_root, [&](auto* n) { return n->is_visible() && n->hit_test(e.pos); }, [&](auto* n) { v = n; });
        if (m_hovered != v) {
            if (m_hovered) {
                m_hovered->on_mouse_leave();
            }
            m_hovered = v;
            if (m_hovered) {
                m_hovered->on_mouse_enter();
            }
        }

        if (v) {
            v->on_mouse_move(e);
        }
    }

    void root_view::mouse_up(const mouse_button_event_args& e) noexcept
    {
        if (!m_root) {
            return;
        }

        view* v = nullptr;
        traverse_preorder(m_root, [&](auto* n) { return n->is_visible() && n->hit_test(e.pos); }, [&](auto* n) { v = n; });
        if (v) {
            v->on_mouse_up(e);
        }
    }

} // namespace tavros::ui

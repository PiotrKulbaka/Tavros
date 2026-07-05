#pragma once

#include <tavros/renderer/render_system.hpp>
#include <tavros/ui/base.hpp>
#include <tavros/ui/view.hpp>

namespace tavros::ui
{

    class root_view
    {
    public:
        root_view(core::shared_ptr<renderer::render_system> rs);

        ~root_view() noexcept;

        void set_root(view* root) noexcept
        {
            m_root = root;
        }

        void process_input(input::event_args_queue_view events) noexcept;

        void resolve();

        void layout(geometry::aabb2 bounds) noexcept;

        void update(float dt) noexcept;

        void draw();

    private:
        void mouse_down(const mouse_button_event_args& e) noexcept;
        void mouse_move(const mouse_move_event_args& e) noexcept;
        void mouse_up(const mouse_button_event_args& e) noexcept;

    private:
        core::shared_ptr<renderer::render_system> m_rs;

        view* m_root = nullptr;           // Always created automatically
        view* m_hovered = nullptr;        // Currently hovered view (by mouse)
        view* m_keyboard_focus = nullptr; // Currently focused view (by keyboard)
        bool  m_is_active = false;        // Whether the application is active (receiving input)
    };

} // namespace tavros::ui

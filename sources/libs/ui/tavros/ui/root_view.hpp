#pragma once

#include <tavros/core/memory/mallocator.hpp>
#include <tavros/ui/base.hpp>
#include <tavros/ui/view.hpp>

namespace tavros::ui
{

    class root_view
    {
    public:
        root_view();

        ~root_view();

        // Initialize with graphics device (for lazy init)
        void init(rhi::graphics_device* gdevice);

        // Shutdown all graphics device resources
        void shutdown();


        view& root()
        {
            return m_root;
        }


        void process_ui_events(input::event_args_queue_view events);

        void on_frame(rhi::command_queue* cmds, float delta_time);

    private:
        void update(float delta_time);

        void layout();

        void render(rhi::command_queue* cmds);

        void on_mouse_down(const mouse_button_event_args&);
        void on_mouse_move(const mouse_move_event_args&);
        void on_mouse_up(const mouse_button_event_args&);
        void on_mouse_wheel(const mouse_wheel_event_args&);
        void on_key_down(const keyboard_key_event_args&);
        void on_key_up(const keyboard_key_event_args&);
        void on_key_press(const key_press_event_args&);

    private:
        rhi::graphics_device*    m_graphics_device;
        renderer::debug_renderer m_debug_renderer;
        bool                     m_is_active;
        view                     m_root; // Always created automatically
        view*                    m_keyboard_focus;
        view*                    m_hovered;
        math::mat4               m_orto_proj;
    };

} // namespace tavros::ui

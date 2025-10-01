#pragma once

#include <tavros/system/interfaces/window.hpp>

#include <Windows.h>

namespace tavros::system
{

    class window : public interfaces::window, core::noncopyable, core::nonmovable
    {
    public:
        window(core::string_view name);
        virtual ~window() override;

        virtual void set_text(core::string_view text) override;

        virtual auto get_window_state() -> window_state override;
        virtual void set_window_state(window_state ws) override;
        virtual auto get_window_size() -> math::isize2 override;
        virtual void set_window_size(int32 width, int32 height) override;
        virtual auto get_location() -> math::ipoint2 override;
        virtual void set_location(int32 left, int32 top) override;
        virtual auto get_client_size() -> math::isize2 override;
        virtual void set_client_size(int32 width, int32 height) override;
        virtual bool is_enabled() override;
        virtual void set_enabled(bool enable) override;

        virtual void activate() override;
        virtual void show() override;
        virtual void hide() override;
        virtual void close() override;

        virtual void set_on_close_listener(close_callback cb) override;
        virtual void set_on_activate_listener(event_callback cb) override;
        virtual void set_on_deactivate_listener(event_callback cb) override;
        virtual void set_on_drop_listener(drop_callback cb) override;
        virtual void set_on_move_listener(move_callback cb) override;
        virtual void set_on_resize_listener(size_callback cb) override;
        virtual void set_on_mouse_down_listener(mouse_callback cb) override;
        virtual void set_on_mouse_move_listener(mouse_callback cb) override;
        virtual void set_on_mouse_up_listener(mouse_callback cb) override;
        virtual void set_on_mouse_wheel_listener(mouse_callback cb) override;
        virtual void set_on_key_down_listener(key_callback cb) override;
        virtual void set_on_key_up_listener(key_callback cb) override;
        virtual void set_on_key_press_listener(key_callback cb) override;

        virtual void on_close(close_event_args& e) override;
        virtual void on_activate() override;
        virtual void on_deactivate() override;
        virtual void on_drop(drop_event_args& e) override;
        virtual void on_move(move_event_args& e) override;
        virtual void on_resize(size_event_args& e) override;
        virtual void on_mouse_down(mouse_event_args& e) override;
        virtual void on_mouse_move(mouse_event_args& e) override;
        virtual void on_mouse_up(mouse_event_args& e) override;
        virtual void on_mouse_wheel(mouse_event_args& e) override;
        virtual void on_key_down(key_event_args& e) override;
        virtual void on_key_up(key_event_args& e) override;
        virtual void on_key_press(key_event_args& e) override;

        virtual void* get_handle() const override;

    public:
        long process_window_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    protected:
        HWND           m_hWnd;
        close_callback m_on_close_cb;
        event_callback m_on_activate_cb;
        event_callback m_on_deactivate_cb;
        drop_callback  m_on_drop_cb;
        move_callback  m_on_move_cb;
        size_callback  m_on_resize_cb;
        mouse_callback m_on_mouse_down_cb;
        mouse_callback m_on_mouse_move_cb;
        mouse_callback m_on_mouse_up_cb;
        mouse_callback m_on_mouse_wheel_cb;
        key_callback   m_on_key_down_cb;
        key_callback   m_on_key_up_cb;
        key_callback   m_on_key_press_cb;
    };

} // namespace tavros::system

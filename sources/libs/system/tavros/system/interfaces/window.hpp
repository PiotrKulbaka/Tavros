#pragma once

#include <tavros/core/prelude.hpp>
#include <tavros/core/math/ivec2.hpp>

#include <tavros/system/event_callbacks.hpp>
#include <tavros/system/window_state.hpp>

namespace tavros::system
{
    using window_uptr = tavros::core::unique_ptr<interfaces::window>;
} // namespace tavros::system

namespace tavros::system::interfaces
{
    class window
    {
    public:
        static auto create(core::string_view name) -> window_uptr;

    public:
        virtual ~window() = default;

        virtual void set_text(core::string_view text) = 0;

        virtual auto get_window_state() -> window_state = 0;
        virtual void set_window_state(window_state ws) = 0;
        virtual auto get_window_size() -> math::isize2 = 0;
        virtual void set_window_size(int32 width, int32 height) = 0;
        virtual auto get_location() -> math::ipoint2 = 0;
        virtual void set_location(int32 left, int32 top) = 0;
        virtual auto get_client_size() -> math::isize2 = 0;
        virtual void set_client_size(int32 width, int32 height) = 0;
        virtual bool is_enabled() = 0;
        virtual void set_enabled(bool enable) = 0;

        virtual void activate() = 0;
        virtual void show() = 0;
        virtual void hide() = 0;
        virtual void close() = 0;

        virtual void set_on_close_listener(close_callback cb) = 0;
        virtual void set_on_activate_listener(event_callback cb) = 0;
        virtual void set_on_deactivate_listener(event_callback cb) = 0;
        virtual void set_on_drop_listener(drop_callback cb) = 0;
        virtual void set_on_move_listener(move_callback cb) = 0;
        virtual void set_on_resize_listener(size_callback cb) = 0;
        virtual void set_on_mouse_down_listener(mouse_callback cb) = 0;
        virtual void set_on_mouse_move_listener(mouse_callback cb) = 0;
        virtual void set_on_mouse_up_listener(mouse_callback cb) = 0;
        virtual void set_on_mouse_wheel_listener(mouse_callback cb) = 0;
        virtual void set_on_key_down_listener(key_callback cb) = 0;
        virtual void set_on_key_up_listener(key_callback cb) = 0;
        virtual void set_on_key_press_listener(key_callback cb) = 0;

        virtual void on_close(close_event_args& e) = 0;
        virtual void on_activate() = 0;
        virtual void on_deactivate() = 0;
        virtual void on_drop(drop_event_args& e) = 0;
        virtual void on_move(move_event_args& e) = 0;
        virtual void on_resize(size_event_args& e) = 0;
        virtual void on_mouse_down(mouse_event_args& e) = 0;
        virtual void on_mouse_move(mouse_event_args& e) = 0;
        virtual void on_mouse_up(mouse_event_args& e) = 0;
        virtual void on_mouse_wheel(mouse_event_args& e) = 0;
        virtual void on_key_down(key_event_args& e) = 0;
        virtual void on_key_up(key_event_args& e) = 0;
        virtual void on_key_press(key_event_args& e) = 0;

        virtual void* get_handle() const = 0;
    };
} // namespace tavros::system::interfaces

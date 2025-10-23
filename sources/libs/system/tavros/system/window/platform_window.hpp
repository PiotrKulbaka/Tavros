#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/math/ivec2.hpp>

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/nonmovable.hpp>
#include <tavros/system/event_args.hpp>
#include <tavros/system/window_state.hpp>
#include <functional>

namespace tavros::system
{

    class platform_window;
    using mouse_callback = std::function<void(platform_window*, mouse_event_args& e)>;
    using key_callback = std::function<void(platform_window*, key_event_args& e)>;
    using move_callback = std::function<void(platform_window*, move_event_args& e)>;
    using size_callback = std::function<void(platform_window*, size_event_args& e)>;
    using close_callback = std::function<void(platform_window*, close_event_args& e)>;
    using drop_callback = std::function<void(platform_window*, drop_event_args& e)>;
    using event_callback = std::function<void(platform_window*)>;

    class platform_window : core::noncopyable, core::nonmovable
    {
    public:
        static tavros::core::unique_ptr<platform_window> create(core::string_view name);

    public:
        virtual ~platform_window() = default;

        virtual void set_text(core::string_view text) = 0;

        virtual void         set_state(window_state ws) = 0;
        virtual window_state state() const = 0;

        virtual void          set_location(int32 left, int32 top) = 0;
        virtual math::ipoint2 location() const = 0;

        virtual void         set_size(int32 width, int32 height) = 0;
        virtual math::isize2 size() const = 0;

        virtual void         set_client_size(int32 width, int32 height) = 0;
        virtual math::isize2 client_size() const = 0;

        virtual void set_enabled(bool enable) = 0;
        virtual bool is_enabled() const = 0;

        virtual void activate() = 0;
        virtual void show() = 0;
        virtual void hide() = 0;
        virtual void close() = 0;

        virtual void* native_handle() const noexcept = 0;

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
    };
} // namespace tavros::system

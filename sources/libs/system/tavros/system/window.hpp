#pragma once

#include <tavros/core/string.hpp>
#include <tavros/system/event_args.hpp>
#include <tavros/system/window/platform_window.hpp>

namespace tavros::system
{
    class window : core::noncopyable
    {
    public:
        static tavros::core::unique_ptr<window> create(core::string_view name);

    public:
        window(core::string_view title);
        virtual ~window();

        void create();
        void destroy();

        void              set_title(core::string_view title);
        core::string_view title() const;

        void         set_state(window_state ws);
        window_state state() const;

        void          set_position(int32 left, int32 top);
        math::ipoint2 position() const;

        void         set_size(int32 width, int32 height);
        math::isize2 size() const;

        void         set_client_size(int32 width, int32 height);
        math::isize2 client_size() const;

        void set_enabled(bool enable);
        bool is_enabled() const;

        void activate();
        void show();
        void hide();
        void close();

        void* native_handle() const noexcept;

        virtual void on_close(close_event_args& e);
        virtual void on_activate();
        virtual void on_deactivate();
        virtual void on_drop(drop_event_args& e);
        virtual void on_move(move_event_args& e);
        virtual void on_resize(size_event_args& e);
        virtual void on_mouse_down(mouse_event_args& e);
        virtual void on_mouse_move(mouse_event_args& e);
        virtual void on_mouse_up(mouse_event_args& e);
        virtual void on_mouse_wheel(mouse_event_args& e);
        virtual void on_key_down(key_event_args& e);
        virtual void on_key_up(key_event_args& e);
        virtual void on_key_press(key_event_args& e);

    private:
        core::unique_ptr<platform_window> m_wnd;
        core::string                      m_title;
    };
} // namespace tavros::system

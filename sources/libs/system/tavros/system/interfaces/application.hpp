#pragma once

#include <tavros/core/prelude.hpp>
#include <tavros/core/math/ivec2.hpp>

namespace tavros::system::interfaces
{
    class application;
}

namespace tavros::system
{
    using application_uptr = tavros::core::unique_ptr<interfaces::application>;
} // namespace tavros::system

namespace tavros::system::interfaces
{

    class application
    {
    public:
        static auto create() -> application_uptr;

    public:
        virtual ~application() = default;

        virtual void run() = 0;
        virtual void exit() = 0;
        virtual bool is_runing() = 0;

        virtual void poll_events() = 0;

        virtual void wait_events() = 0;

        virtual math::isize2 desktop_size() = 0;

        // virtual void add_window(window_ptr wnd) = 0;
        // virtual void remove_window(window_ptr wnd) = 0;
        // virtual void set_main_window(window_ptr wnd) = 0;
        // virtual auto get_main_window() -> window_ptr = 0;

        // virtual auto get_clipboard_text() -> core::string = 0;
        // virtual void set_clipboard_text(core::string_view text) = 0;
    };

} // namespace tavros::system::interfaces

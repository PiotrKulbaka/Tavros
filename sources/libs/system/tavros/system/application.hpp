#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/nonmovable.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/math/ivec2.hpp>

namespace tavros::system
{

    class application : core::noncopyable, core::nonmovable
    {
    public:
        static tavros::core::unique_ptr<application> create();

    public:
        virtual ~application() = default;

        virtual int  run() = 0;
        virtual void exit(int exit_code) = 0;

        virtual void poll_events() = 0;
        virtual void wait_events() = 0;

        virtual math::isize2 desktop_size() = 0;

        // virtual auto get_clipboard_text() -> core::string = 0;
        // virtual void set_clipboard_text(core::string_view text) = 0;
    };

} // namespace tavros::system

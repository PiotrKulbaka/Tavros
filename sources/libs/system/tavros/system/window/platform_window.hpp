#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/math/ivec2.hpp>

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/nonmovable.hpp>
#include <tavros/system/event_args.hpp>
#include <tavros/system/window_state.hpp>

namespace tavros::system
{
    class window;

    class platform_window : core::noncopyable, core::nonmovable
    {
    public:
        static tavros::core::unique_ptr<platform_window> create(core::string_view name);

    public:
        virtual ~platform_window() = default;

        virtual void          set_text(core::string_view text) = 0;
        virtual void          set_state(window_state ws) = 0;
        virtual window_state  get_state() const = 0;
        virtual void          set_position(int32 left, int32 top) = 0;
        virtual math::ipoint2 get_position() const = 0;
        virtual void          set_size(int32 width, int32 height, bool client_rect) = 0;
        virtual math::isize2  get_size(bool client_rect) const = 0;

        virtual void set_enabled(bool enable) = 0;
        virtual bool is_enabled() const = 0;

        virtual void activate() = 0;
        virtual void show() = 0;
        virtual void hide() = 0;
        virtual void close() = 0;

        virtual void* native_handle() const noexcept = 0;

        virtual void set_events_listener(window* listener) = 0;
    };
} // namespace tavros::system

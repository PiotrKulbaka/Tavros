#pragma once

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/ivec2.hpp>
#include <tavros/system/keys.hpp>
#include <functional>

namespace tavros::system::interfaces
{
    class window;
} // namespace tavros::system::interfaces

namespace tavros::system
{

    struct mouse_event_args
    {
        mouse_button button = mouse_button::none;
        bool         is_double_click = false;  // Can be true only in on_mouse_down event
        bool         is_relative_move = false; // If 'true' then 'pos' stores in relative otherwise absolute units
        math::point2 delta;                    // Wheel delta
        math::point2 pos;                      // Move position, stores in absolute or relative units
    };

    struct key_event_args
    {
        keys   key = keys::none;        // Current key; for on_key_press is none
        bool   is_prev_pressed = false; // true if the key was pressed last time
        uint16 repeats = 0;             // Number of clicks
        int32  key_char = 0;            // Only in on_key_press event
    };

    struct move_event_args
    {
        math::ipoint2 pos;
    };

    struct size_event_args
    {
        math::isize2 size;
    };

    struct close_event_args
    {
        bool cancel = false; // Cancel closing
    };

    struct drop_event_args
    {
        const char** files = nullptr; // List of files ending with nullptr
    };

    using window_ptr = interfaces::window*;
    using mouse_callback = std::function<void(window_ptr, mouse_event_args& e)>;
    using key_callback = std::function<void(window_ptr, key_event_args& e)>;
    using move_callback = std::function<void(window_ptr, move_event_args& e)>;
    using size_callback = std::function<void(window_ptr, size_event_args& e)>;
    using close_callback = std::function<void(window_ptr, close_event_args& e)>;
    using drop_callback = std::function<void(window_ptr, drop_event_args& e)>;
    using event_callback = std::function<void(window_ptr)>;

} // namespace tavros::system

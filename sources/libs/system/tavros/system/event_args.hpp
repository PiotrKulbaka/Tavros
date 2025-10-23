#pragma once

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/ivec2.hpp>
#include <tavros/system/keys.hpp>

namespace tavros::system
{

    /**
     * @brief Contains information about a mouse event.
     */
    struct mouse_event_args
    {
        uint64       event_time_us = 0;           /// Event timestamp in microseconds (use get_high_precision_system_time_us() for current time).
        mouse_button button = mouse_button::none; /// Mouse button associated with the event.
        bool         is_double_click = false;     /// True only for a double-click in an on_mouse_down event.
        bool         is_relative_move = false;    /// True if 'pos' represents relative movement instead of absolute.
        math::point2 delta;                       /// Wheel movement delta.
        math::point2 pos;                         /// Mouse position, either absolute or relative.
    };

    /**
     * @brief Contains information about a keyboard event.
     */
    struct key_event_args
    {
        uint64 event_time_us = 0;       /// Event timestamp in microseconds (use get_high_precision_system_time_us() for current time).
        keys   key = keys::none;        /// Key associated with the event; 'none' for on_key_press.
        bool   is_prev_pressed = false; /// True if the key was pressed previously.
        uint16 repeats = 0;             /// Number of repeated presses.
        int32  key_char = 0;            /// Unicode character code (valid only for on_key_press).
    };

    /**
     * @brief Contains information about a window move event.
     */
    struct move_event_args
    {
        uint64        event_time_us = 0; /// Event timestamp in microseconds (use get_high_precision_system_time_us() for current time).
        math::ipoint2 pos;               /// New window position.
    };

    /**
     * @brief Contains information about a window resize event.
     */
    struct size_event_args
    {
        uint64       event_time_us = 0; /// Event timestamp in microseconds (use get_high_precision_system_time_us() for current time).
        math::isize2 size;              /// New window size.
    };

    /**
     * @brief Contains information about a window close event.
     */
    struct close_event_args
    {
        uint64 event_time_us = 0; /// Event timestamp in microseconds (use get_high_precision_system_time_us() for current time).
        bool   cancel = false;    /// Set to true to cancel the closing action.
    };

    /**
     * @brief Contains information about a file drop event.
     */
    struct drop_event_args
    {
        uint64       event_time_us = 0; /// Event timestamp in microseconds (use get_high_precision_system_time_us() for current time).
        const char** files = nullptr;   /// Null-terminated list of dropped file paths.
    };

} // namespace tavros::system

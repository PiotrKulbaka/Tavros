#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/math/vec2.hpp>
#include <tavros/core/memory/buffer_view.hpp>
#include <tavros/input/keyboard_key.hpp>
#include <tavros/input/mouse_button.hpp>

namespace tavros::input
{

    /**
     * @brief Enumerates the types of input events.
     */
    enum class event_type : uint8
    {
        none,             /// No event (default value)
        mouse_down,       /// Mouse button pressed
        mouse_move,       /// Mouse moved (absolute position)
        mouse_move_delta, /// Mouse moved (relative delta, raw mouse input)
        mouse_up,         /// Mouse button released
        mouse_wheel,      /// Mouse wheel scrolled
        key_down,         /// Key pressed down
        key_press,        /// Key held or repeated press
        key_up,           /// Key released
        window_size,      /// Window resized
        window_move,      /// Window moved
        activate,         /// Input system or window activated
        deactivate,       /// Input system or window deactivated
    };

    /**
     * @brief Holds the data for a single input event.
     */
    struct event_args
    {
        /// Type of the event
        event_type type = event_type::none;

        /// Keyboard key (for keyboard events)
        keyboard_key key = keyboard_key::none;

        /// Mouse button (for mouse events)
        mouse_button button = mouse_button::none;

        /// General-purpose 2D vector (e.g. position or delta)
        math::vec2 vec;

        /// Timestamp of the event in microseconds
        uint64 time_us = 0;
    };

    /// Type alias for accessing the front queue as a contiguous view.
    using event_args_queue_view = tavros::core::buffer_view<event_args>;

} // namespace tavros::input

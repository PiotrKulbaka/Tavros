#pragma once

#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/string.hpp>
#include <tavros/renderer/render_system.hpp>

#include <tavros/input/event_args.hpp>

namespace tavros::ui
{

    struct render_context
    {
        renderer::render_system* renderer = nullptr;
        bool                     debug_draw = false;
    };

    using mouse_button = input::mouse_button;
    using keyboard_key = input::keyboard_key;

    struct mouse_button_event_args
    {
        uint64       time_us = 0;
        mouse_button button = mouse_button::none;
        math::vec2   pos;
    };

    struct mouse_move_event_args
    {
        uint64     time_us = 0;
        math::vec2 pos;
    };

    struct mouse_wheel_event_args
    {
        uint64     time_us = 0;
        math::vec2 wheel_delta;
        math::vec2 pos;
    };

    struct keyboard_key_event_args
    {
        uint64       time_us = 0;
        keyboard_key key = keyboard_key::none;
    };

    struct key_press_event_args
    {
        uint64 time_us = 0;
        uint16 repeats = 0;
        uint32 key_char = 0;
    };

    struct layout_constraints
    {
        math::vec2 min_size;
        math::vec2 max_size;
    };

} // namespace tavros::ui

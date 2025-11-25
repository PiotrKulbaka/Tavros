#pragma once

#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/string.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/debug_renderer.hpp>

#include <tavros/input/event_args.hpp>

namespace tavros::ui
{

    namespace rhi = renderer::rhi;

    using vec2 = math::vec2;
    using size2 = math::size2;
    using point2 = math::point2;
    using padding2 = geometry::aabb2;
    using isize2 = math::isize2;
    using vec3 = math::vec3;
    using vec4 = math::vec4;

    using rect2 = geometry::aabb2;

    struct render_context
    {
        rhi::graphics_device*     gdevice = nullptr;
        renderer::debug_renderer* drenderer = nullptr;
    };

    using mouse_button = input::mouse_button;
    using keyboard_key = input::keyboard_key;


    struct mouse_button_event_args
    {
        uint64       time_us = 0;
        mouse_button button = mouse_button::none;
        point2       pos;
    };

    struct mouse_move_event_args
    {
        uint64 time_us = 0;
        point2 pos;
    };

    struct mouse_wheel_event_args
    {
        uint64 time_us = 0;
        size2  wheel_delta;
        point2 pos;
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
        size2 min_size;
        size2 max_size;
    };

} // namespace tavros::ui

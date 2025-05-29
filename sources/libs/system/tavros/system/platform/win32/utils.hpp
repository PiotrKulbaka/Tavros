#pragma once

#include <tavros/system/event_callbacks.hpp>
#include <tavros/system/keys.hpp>

#include <tavros/core/math/ivec2.hpp>

#include <Windows.h>

namespace tavros::system
{
    const char*      wm_message_to_str(UINT msg);
    keys             map_key(UINT vk);
    const char*      last_win_error_str(); // do not store a pointer to the str
    math::ipoint2    create_point2(LPARAM lParam);
    math::isize2     create_size2(LPARAM lParam);
    mouse_event_args create_mouse_event_args(LPARAM lParam, mouse_button btn, bool double_click = false, int32 delta = 0);
    mouse_button     create_mouse_x_button(WPARAM wParam);
    key_event_args   create_key_event_args(WPARAM wParam, LPARAM lParam);
    key_event_args   create_char_event_args(WPARAM wParam, LPARAM lParam);
} // namespace tavros::system

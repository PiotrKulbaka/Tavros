#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::system
{
    enum class keys : uint8
    {
        none = 0,
        k_lbutton,      // Left mouse button
        k_rbutton,      // Right mouse button
        k_break,        // Control-break processing
        k_mbutton,      // Middle mouse button
        k_xbutton1,     // X1 mouse button
        k_xbutton2,     // X2 mouse button
        k_backspace,    // Backspace key
        k_tab,          // Tab key
        k_enter,        // Enter key
        k_pause,        // Pause key
        k_capslock,     // Caps lock key
        k_escape,       // Esc key
        k_space,        // SPACEBAR
        k_pageup,       // Page up key
        k_pagedown,     // Page down key
        k_end,          // End key
        k_home,         // Home key
        k_left,         // Left arrow key
        k_up,           // Up arrow key
        k_right,        // Right arrow key
        k_down,         // Down arrow key
        k_print,        // Print key
        k_print_screen, // Print screen key
        k_insert,       // Ins key
        k_delete,       // Del key
        k_help,         // Help key
        k_0,
        k_1,
        k_2,
        k_3,
        k_4,
        k_5,
        k_6,
        k_7,
        k_8,
        k_9,
        k_A,
        k_B,
        k_C,
        k_D,
        k_E,
        k_F,
        k_G,
        k_H,
        k_I,
        k_J,
        k_K,
        k_L,
        k_M,
        k_N,
        k_O,
        k_P,
        k_Q,
        k_R,
        k_S,
        k_T,
        k_U,
        k_V,
        k_W,
        k_X,
        k_Y,
        k_Z,
        k_lsuper,   // Left Windows/Command/Super key
        k_rsuper,   // Right Windows/Command/Super key
        k_menu,     // Applications key
        k_numpad0,  // Numeric keypad 0 key
        k_numpad1,  // Numeric keypad 1 key
        k_numpad2,  // Numeric keypad 2 key
        k_numpad3,  // Numeric keypad 3 key
        k_numpad4,  // Numeric keypad 4 key
        k_numpad5,  // Numeric keypad 5 key
        k_numpad6,  // Numeric keypad 6 key
        k_numpad7,  // Numeric keypad 7 key
        k_numpad8,  // Numeric keypad 8 key
        k_numpad9,  // Numeric keypad 9 key
        k_multiply, // Multiply key
        k_add,      // Add key
        k_subtract, // Subtract key
        k_decimal,  // Decimal key
        k_divide,   // Divide key
        k_F1,
        k_F2,
        k_F3,
        k_F4,
        k_F5,
        k_F6,
        k_F7,
        k_F8,
        k_F9,
        k_F10,
        k_F11,
        k_F12,
        k_F13,
        k_F14,
        k_F15,
        k_F16,
        k_F17,
        k_F18,
        k_F19,
        k_F20,
        k_semicolon,    // `:;` key
        k_equal,        // `+=` key
        k_comma,        // `<,` key
        k_minus,        // `_-` key
        k_period,       // `>.` key
        k_slash,        // `?/` key
        k_grave_accent, // `~`` key
        k_lbracket,     // `{[` key
        k_backslash,    // `|\` key
        k_rbracket,     // `}]` key
        k_apostrophe,   // `"'` key
        k_numlock,      // Num lock key
        k_scroll,       // Scroll lock key
        k_lshift,       // Left Shift key
        k_rshift,       // Right Shift key
        k_lcontrol,     // Left Ctrl key
        k_rcontrol,     // Right Ctrl key
        k_lalt,         // Left Alt key
        k_ralt,         // Right Alt key
        k_last_key,
    };

    enum class mouse_button : uint8
    {
        none = 0,
        left,
        right,
        middle,
        x_button1,
        x_button2,
    };

    core::string_view to_string(keys k) noexcept;
    core::string_view to_string(mouse_button bm) noexcept;

} // namespace tavros::system

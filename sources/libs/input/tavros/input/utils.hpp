#pragma once

#include <tavros/input/keyboard_key.hpp>
#include <tavros/input/mouse_button.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::input
{

    core::string_view to_string(keyboard_key key) noexcept;
    core::string_view to_string(mouse_button btn) noexcept;

} // namespace tavros::input

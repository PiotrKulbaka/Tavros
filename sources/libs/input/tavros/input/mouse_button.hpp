#pragma once

#include <tavros/core/types.hpp>

namespace tavros::input
{

    enum class mouse_button : uint8
    {
        none = 0,
        left,
        right,
        middle,
        x_button1,
        x_button2,
        last_button
    };

} // namespace tavros::input

#pragma once

#include <tavros/core/types.hpp>

namespace tavros::resources
{
    enum class seek_dir : uint8
    {
        begin,
        current,
        end,
    };
}
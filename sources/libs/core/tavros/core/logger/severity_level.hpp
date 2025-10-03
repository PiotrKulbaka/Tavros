#pragma once

#include <tavros/core/types.hpp>

namespace tavros::core
{

    enum class severity_level : uint32
    {
        debug,
        info,
        warning,
        error,
        fatal,
    };

} // namespace tavros::core

#pragma once

#include <tavros/core/types.hpp>

namespace tavros::core
{
    /// @brief Represents the type used for indices.
    using index_type = uint32;

    /// @brief Special constant representing an invalid or uninitialized index.
    constexpr index_type invalid_index = 0xffffffffui32;

} // namespace tavros::core

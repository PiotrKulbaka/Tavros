#pragma once

#include <tavros/core/types.hpp>

namespace tavros::core
{
    /// @brief Represents the type used for indices.
    using index_type = uint64;

    /// @brief Special constant representing an invalid or uninitialized index.
    constexpr index_type invalid_index = 0xffffffffffffffffui64;

} // namespace tavros::core

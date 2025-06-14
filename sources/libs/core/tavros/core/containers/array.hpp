#pragma once

#include <array>

namespace tavros::core
{
    template<typename T, std::size_t N>
    using array = std::array<T, N>;
} // namespace tavros::core

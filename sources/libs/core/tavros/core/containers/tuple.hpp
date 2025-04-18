#pragma once

#include <tuple>

namespace tavros::core
{
    template<typename... Ts>
    using set = std::array<Ts...>;
} // namespace tavros::core

#pragma once

#include <optional>

namespace tavros::core
{
    template<typename T>
    using optional = std::optional<T>;
    inline constexpr std::nullopt_t nullopt = std::nullopt;
} // namespace tavros::core

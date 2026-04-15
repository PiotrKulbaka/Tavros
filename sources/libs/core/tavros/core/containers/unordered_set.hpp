#pragma once

#include <unordered_set>

namespace tavros::core
{
    template<class T, class Hasher = std::hash<T>, class KeyEq = std::equal_to<T>>
    using unordered_set = std::unordered_set<T, Hasher, KeyEq>;
} // namespace tavros::core

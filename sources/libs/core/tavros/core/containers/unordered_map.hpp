#pragma once

#include <unordered_map>

namespace tavros::core
{
    template<class Key, class Value, class Hasher = std::hash<Key>, class Eq = std::equal_to<Key>>
    using unordered_map = std::unordered_map<Key, Value, Hasher, Eq>;
} // namespace tavros::core

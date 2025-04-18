#pragma once

#include <unordered_map>

namespace tavros::core
{
    template<typename K, typename V>
    using unordered_map = std::unordered_map<K, V>;
} // namespace tavros::core

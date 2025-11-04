#pragma once

#include <unordered_map>

namespace tavros::core
{
    template<class K, class V, class H = std::hash<K>>
    using unordered_map = std::unordered_map<K, V, H>;
} // namespace tavros::core

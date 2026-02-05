#pragma once

#include <map>

namespace tavros::core
{
    template<class K, class V, class C = std::less<K>>
    using map = std::map<K, V, C>;
} // namespace tavros::core

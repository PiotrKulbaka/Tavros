#pragma once

#include <set>

namespace tavros::core
{
    template<class Type, class Compare = std::less<Type>>
    using set = std::set<Type, Compare>;
} // namespace tavros::core

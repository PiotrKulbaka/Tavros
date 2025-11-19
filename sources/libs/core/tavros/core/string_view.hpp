#pragma once

#include <string_view>

namespace tavros::core
{
    using string_view = std::basic_string_view<char>;
    using u32string_view = std::basic_string_view<char32_t>;
} // namespace tavros::core

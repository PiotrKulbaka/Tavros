#pragma once

#include <string>

namespace tavros::core
{
    using string = std::basic_string<char, std::char_traits<char> /*, std::allocator<char>*/>;
    using u32string = std::basic_string<char32_t, std::char_traits<char32_t> /*, std::allocator<char32_t>*/>;
} // namespace tavros::core

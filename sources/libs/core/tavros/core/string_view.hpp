#pragma once

#include <string_view>

namespace tavros::core
{
    using string_view = std::basic_string_view<char>;
    using u32string_view = std::basic_string_view<char32_t>;

    constexpr string_view operator""_sv(const char* str, size_t len) noexcept
    {
        return string_view{str, len};
    }

    constexpr string_view operator""_sv(const char* str) noexcept
    {
        return string_view{str};
    }
} // namespace tavros::core

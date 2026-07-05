#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/fixed_string.hpp>

namespace tavros::core
{

    /**
     * @brief Transparent comparator for ordered containers (map, set).
     * Enables lookup by string_view without constructing a temporary string.
     */
    struct string_less
    {
        using is_transparent = void;

        template<typename A, typename B>
        bool operator()(const A& a, const B& b) const noexcept
        {
            return to_view(a) < to_view(b);
        }

    private:
        static string_view to_view(string_view s) noexcept
        {
            return s;
        }

        static string_view to_view(const string& s) noexcept
        {
            return string_view(s);
        }

        template<size_t N>
        static string_view to_view(const fixed_string<N>& s) noexcept
        {
            return string_view(s);
        }
    };

    /**
     * @brief Transparent hasher for unordered containers.
     * Enables lookup by string_view without constructing a temporary string.
     */
    struct string_hash
    {
        using is_transparent = void;

        size_t operator()(string_view sv) const noexcept
        {
            return std::hash<string_view>{}(sv);
        }

        size_t operator()(const string& s) const noexcept
        {
            return std::hash<string_view>{}(s);
        }
    };

    /**
     * @brief Transparent equality comparator for unordered containers.
     * Intended to be used together with string_hash.
     */
    struct string_equal
    {
        using is_transparent = void;

        bool operator()(string_view a, string_view b) const noexcept
        {
            return a == b;
        }
    };

} // namespace tavros::core

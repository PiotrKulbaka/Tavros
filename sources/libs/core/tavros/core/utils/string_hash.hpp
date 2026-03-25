#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::core
{

    /**
     * @brief Transparent comparator for ordered containers (map, set).
     * Enables lookup by string_view without constructing a temporary string.
     */
    struct string_less
    {
        using is_transparent = void;

        bool operator()(const string& a, const string& b) const noexcept
        {
            return a < b;
        }

        bool operator()(const string& a, string_view b) const noexcept
        {
            return a < b;
        }

        bool operator()(string_view a, const string& b) const noexcept
        {
            return a < b;
        }

        bool operator()(string_view a, string_view b) const noexcept
        {
            return a < b;
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

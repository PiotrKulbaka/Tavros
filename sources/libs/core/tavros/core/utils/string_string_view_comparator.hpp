#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::core
{

    struct string_string_view_comparator
    {
        using is_transparent = void;
        bool operator()(const tavros::core::string& a, const tavros::core::string& b) const
        {
            return a < b;
        }
        bool operator()(const tavros::core::string& a, tavros::core::string_view b) const
        {
            return a < b;
        }
        bool operator()(tavros::core::string_view a, const tavros::core::string& b) const
        {
            return a < b;
        }
    };

} // namespace tavros::core

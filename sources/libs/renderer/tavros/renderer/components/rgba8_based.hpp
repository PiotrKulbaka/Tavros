#pragma once

#include <tavros/core/math/rgba8.hpp>

namespace tavros::renderer
{

    struct primary_color_c : public math::rgba8
    {
        constexpr primary_color_c& operator=(const math::rgba8& other) noexcept
        {
            math::rgba8::operator=(other);
            return *this;
        }
    };

    struct outline_color_c : public math::rgba8
    {
        constexpr outline_color_c& operator=(const math::rgba8& other) noexcept
        {
            math::rgba8::operator=(other);
            return *this;
        }
    };

} // namespace tavros::renderer
#pragma once

/**
 * @file min.hpp
 */

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>

#ifdef min
    #undef min
#endif

namespace tavros::math
{

    inline constexpr float min(float a, float b) noexcept
    {
        return a < b ? a : b;
    }

    inline constexpr float min(float a, float b, float c) noexcept
    {
        return min(min(a, b), c);
    }

    inline constexpr vec2 min(const vec2& a, const vec2& b) noexcept
    {
        return vec2(min(a.x, b.x), min(a.y, b.y));
    }

    inline constexpr vec3 min(const vec3& a, const vec3& b) noexcept
    {
        return vec3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
    }

    inline constexpr vec4 min(const vec4& a, const vec4& b) noexcept
    {
        return vec4(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w));
    }

} // namespace tavros::math

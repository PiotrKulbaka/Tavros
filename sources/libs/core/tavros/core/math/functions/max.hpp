#pragma once

/**
 * @file max.hpp
 */

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>

#ifdef max
    #undef max
#endif

namespace tavros::math
{

    inline constexpr float max(float a, float b) noexcept
    {
        return a > b ? a : b;
    }

    inline constexpr float max(float a, float b, float c) noexcept
    {
        return max(max(a, b), c);
    }

    inline constexpr vec2 max(const vec2& a, const vec2& b) noexcept
    {
        return vec2(max(a.x, b.x), max(a.y, b.y));
    }

    inline constexpr vec3 max(const vec3& a, const vec3& b) noexcept
    {
        return vec3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
    }

    inline constexpr vec4 max(const vec4& a, const vec4& b) noexcept
    {
        return vec4(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w));
    }

} // namespace tavros::math

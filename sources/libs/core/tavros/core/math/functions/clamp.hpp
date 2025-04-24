#pragma once

/**
 * @file clamp.hpp
 */

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>

namespace tavros::math
{

    template<typename T>
    constexpr T clamp(T value, T min, T max) noexcept
    {
        return value < min ? min : (value > max ? max : value);
    }

    inline constexpr vec2 clamp(const vec2& val, const vec2& min, const vec2& max) noexcept
    {
        return vec2(clamp(val.x, min.x, max.x), clamp(val.y, min.y, max.y));
    }

    inline constexpr vec3 clamp(const vec3& val, const vec3& min, const vec3& max) noexcept
    {
        return vec3(clamp(val.x, min.x, max.x), clamp(val.y, min.y, max.y), clamp(val.z, min.z, max.z));
    }

    inline constexpr vec4 clamp(const vec4& val, const vec4& min, const vec4& max) noexcept
    {
        return vec4(clamp(val.x, min.x, max.x), clamp(val.y, min.y, max.y), clamp(val.z, min.z, max.z), clamp(val.w, min.w, max.w));
    }

} // namespace tavros::math

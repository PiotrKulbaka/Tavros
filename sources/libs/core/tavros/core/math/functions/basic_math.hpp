#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/debug/assert.hpp>

#include <cmath>

namespace tavros::math
{

    constexpr float k_epsilon6 = 1e-6f;
    constexpr float k_pi = 3.1415926535897932384626433832795f;
    constexpr float k_two_pi = static_cast<float>(2.0 * k_pi);
    constexpr float k_half_pi = static_cast<float>(k_pi / 2.0);

    [[nodiscard]] inline constexpr float sin(float rad) noexcept
    {
        return std::sin(rad);
    }

    [[nodiscard]] inline constexpr float cos(float rad) noexcept
    {
        return std::cos(rad);
    }

    [[nodiscard]] inline constexpr float tan(float rad) noexcept
    {
        return std::tan(rad);
    }

    [[nodiscard]] inline constexpr float asin(float value) noexcept
    {
        return std::asin(value);
    }

    [[nodiscard]] inline constexpr float acos(float value) noexcept
    {
        return std::acos(value);
    }

    [[nodiscard]] inline constexpr float atan(float value) noexcept
    {
        return std::atan(value);
    }

    [[nodiscard]] inline constexpr float atan2(float y, float x) noexcept
    {
        return std::atan2(y, x);
    }

    [[nodiscard]] inline constexpr float abs(float value) noexcept
    {
        return std::abs(value);
    }

    [[nodiscard]] inline constexpr float sqrt(float value) noexcept
    {
        return std::sqrt(value);
    }

    [[nodiscard]] inline constexpr bool almost_equal(float a, float b, float epsilon = k_epsilon6) noexcept
    {
        return abs(a - b) <= epsilon;
    }

    [[nodiscard]] inline constexpr bool almost_zero(float value, float epsilon = k_epsilon6) noexcept
    {
        return abs(value) <= epsilon;
    }

    [[nodiscard]] inline constexpr float wrap_angle(float angle) noexcept
    {
        angle = std::fmod(angle + k_pi, k_two_pi);
        if (angle < 0.0f) {
            angle += k_two_pi;
        }
        return angle - k_pi;
    }

    [[nodiscard]] inline constexpr float rad_to_deg(float rad)
    {
        return rad * 180.0f / k_pi;
    }

    [[nodiscard]] inline constexpr float deg_to_rad(float rad)
    {
        return rad * k_pi / 180.0f;
    }

} // namespace tavros::math

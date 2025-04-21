#pragma once

#include <cmath>

namespace tavros::core::math
{
    constexpr double k_pi_d = 3.1415926535897932384626433832795;
    constexpr float  k_epsilon6 = 1e-6f;
    constexpr float  k_pi = static_cast<float>(k_pi_d);
    constexpr float  k_2pi = static_cast<float>(2.0 * k_pi);
    constexpr float  k_pi_div_2 = static_cast<float>(k_pi / 2.0);

    template<class T>
    constexpr T clamp(T value, T min, T max) noexcept
    {
        return value < min ? min : (value > max ? max : value);
    }

    inline constexpr bool almost_equal(float a, float b, float epsilon) noexcept
    {
        return std::abs(a - b) <= epsilon;
    }

    inline constexpr bool almost_zero(float value, float epsilon) noexcept
    {
        return std::abs(value) <= epsilon;
    }

    inline constexpr float wrap_angle(float angle) noexcept
    {
        angle = std::fmod(angle + k_pi, k_2pi);
        if (angle < 0.0f) {
            angle += k_2pi;
        }
        return angle - k_pi;
    }

} // namespace tavros::core::math

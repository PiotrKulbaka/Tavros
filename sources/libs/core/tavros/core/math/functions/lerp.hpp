#pragma once

/**
 * @file lerp.hpp
 * @brief Provides linear interpolation (lerp) functions between two values of various types.
 *
 * This file defines functions for linear interpolation (lerp) between two values. The interpolation
 * coefficient (`coef`) is a scalar in the range [0..1] but can be outside this range for extrapolation.
 * The result is a value that linearly blends between `a` and `b` based on the coefficient.
 *
 * **Interpolation formula:**
 * `lerp(a, b, coef) = a + (b - a) * coef`
 * Where:
 * - `a` is the starting value.
 * - `b` is the ending value.
 * - `coef` is the interpolation factor, with 0.0 representing `a`, 1.0 representing `b`,
 *   and values outside this range performing extrapolation.
 *
 * This operation is commonly used for:
 * - Smooth transitions (e.g., animations)
 * - Blending colors or vectors
 * - Generating intermediate values
 *
 * ### Supported Types:
 * - `float` — Scalar interpolation
 * - `vec2` — 2D vector interpolation
 * - `vec3` — 3D vector interpolation
 * - `vec4` — 4D vector interpolation
 */

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>

namespace tavros::math
{

    inline constexpr float lerp(float a, float b, float coef) noexcept
    {
        return a + (b - a) * coef;
    }

    inline constexpr vec2 lerp(const vec2& a, const vec2& b, float coef) noexcept
    {
        return a + (b - a) * coef;
    }

    inline constexpr vec3 lerp(const vec3& a, const vec3& b, float coef) noexcept
    {
        return a + (b - a) * coef;
    }

    inline constexpr vec4 lerp(const vec4& a, const vec4& b, float coef) noexcept
    {
        return a + (b - a) * coef;
    }

} // namespace tavros::math

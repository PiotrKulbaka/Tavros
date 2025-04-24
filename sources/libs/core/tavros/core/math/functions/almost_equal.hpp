#pragma once

/**
 * @file almost_equal.hpp
 * @brief Contains functions for comparing floating-point values and mathematical objects
 *        (vectors, quaternions, and matrices) for approximate equality.
 *
 * This file provides utility functions to compare various mathematical objects, such as
 * scalars, vectors, quaternions, and matrices, with a tolerance (epsilon) to account
 * for floating-point precision errors.
 *
 * These functions are commonly used in cases where exact equality is unreliable due to
 * small rounding errors in floating-point arithmetic.
 *
 * @note The default epsilon is set to a predefined constant, `k_epsilon6`, but a custom
 *       epsilon can be provided.
 *
 * Supported types:
 * - `float`
 * - `vec2`
 * - `vec3`
 * - `vec4`
 * - `euler3`
 * - `quat`
 * - `mat3`
 * - `mat4`
 */

#include <tavros/core/math/functions/basic_math.hpp>
#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>
#include <tavros/core/math/euler3.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/mat3.hpp>
#include <tavros/core/math/mat4.hpp>

namespace tavros::math
{

    [[nodiscard]] inline constexpr bool almost_equal(const vec2& a, const vec2& b, float epsilon = k_epsilon6) noexcept
    {
        return almost_equal(a.x, b.x, epsilon)
            && almost_equal(a.y, b.y, epsilon);
    }

    [[nodiscard]] inline constexpr bool almost_equal(const vec3& a, const vec3& b, float epsilon = k_epsilon6) noexcept
    {
        return almost_equal(a.x, b.x, epsilon)
            && almost_equal(a.y, b.y, epsilon)
            && almost_equal(a.z, b.z, epsilon);
    }

    [[nodiscard]] inline constexpr bool almost_equal(const vec4& a, const vec4& b, float epsilon = k_epsilon6) noexcept
    {
        return almost_equal(a.x, b.x, epsilon)
            && almost_equal(a.y, b.y, epsilon)
            && almost_equal(a.z, b.z, epsilon)
            && almost_equal(a.w, b.w, epsilon);
    }

    [[nodiscard]] inline constexpr bool almost_equal(const euler3& a, const euler3& b, float epsilon = k_epsilon6) noexcept
    {
        return almost_equal(a.roll, b.roll, epsilon)
            && almost_equal(a.pitch, b.pitch, epsilon)
            && almost_equal(a.yaw, b.yaw, epsilon);
    }

    [[nodiscard]] inline constexpr bool almost_equal(const quat& a, const quat& b, float epsilon = k_epsilon6) noexcept
    {
        return almost_equal(a.x, b.x, epsilon)
            && almost_equal(a.y, b.y, epsilon)
            && almost_equal(a.z, b.z, epsilon)
            && almost_equal(a.w, b.w, epsilon);
    }

    [[nodiscard]] inline constexpr bool almost_equal(const mat3& a, const mat3& b, float epsilon = k_epsilon6) noexcept
    {
        return almost_equal(a[0], b[0], epsilon)
            && almost_equal(a[1], b[1], epsilon)
            && almost_equal(a[2], b[2], epsilon);
    }

    [[nodiscard]] inline constexpr bool almost_equal(const mat4& a, const mat4& b, float epsilon = k_epsilon6) noexcept
    {
        return almost_equal(a[0], b[0], epsilon)
            && almost_equal(a[1], b[1], epsilon)
            && almost_equal(a[2], b[2], epsilon)
            && almost_equal(a[3], b[3], epsilon);
    }

} // namespace tavros::math

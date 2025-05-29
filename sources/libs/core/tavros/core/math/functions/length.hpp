#pragma once

/**
 * @file length.hpp
 * @brief Provides functions to compute the length (magnitude) and squared length of vectors and quaternions.
 *
 * This file defines utility functions for computing:
 * - The **length** (magnitude) of a vector or quaternion, calculated as the square root of its dot product with itself.
 * - The **squared length**, which avoids the square root operation and is often used in performance-sensitive code
 *   where comparison is sufficient.
 *
 * These functions are commonly used in:
 * - Normalization
 * - Distance and proximity calculations
 * - Physics (e.g., force magnitude, motion)
 * - Geometric queries (e.g., bounding volumes, collision)
 *
 * ### Performance Note:
 * - Prefer `squared_length()` when possible to avoid the cost of `sqrt()`.
 *
 * ### Supported Types:
 * - `vec2` — 2D vector
 * - `vec3` — 3D vector
 * - `vec4` — 4D vector
 * - `quat` — Quaternion
 *
 * @see dot.hpp — For the underlying dot product computations used in these functions.
 */

#include <tavros/core/math/functions/basic_math.hpp>
#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>
#include <tavros/core/math/quat.hpp>

namespace tavros::math
{

    inline float length(const vec2& v) noexcept
    {
        return sqrt(v.x * v.x + v.y * v.y);
    }

    inline constexpr float squared_length(const vec2& v) noexcept
    {
        return v.x * v.x + v.y * v.y;
    }

    inline float length(const vec3& v) noexcept
    {
        return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    inline constexpr float squared_length(const vec3& v) noexcept
    {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    inline float length(const vec4& v) noexcept
    {
        return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    }

    inline constexpr float squared_length(const vec4& v) noexcept
    {
        return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    }

    inline float length(const quat& q) noexcept
    {
        return sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    }

    inline constexpr float squared_length(const quat& q) noexcept
    {
        return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    }

} // namespace tavros::math

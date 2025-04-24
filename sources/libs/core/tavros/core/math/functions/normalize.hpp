#pragma once

/**
 * @file normalize.hpp
 * @brief Provides functions to normalize various mathematical objects.
 *
 * This file defines functions for normalizing vectors, quaternions, and angles. For angles,
 * normalization ensures that each angle is within the range [-π, π], wrapping values that exceed this range.
 *
 * ### Normalization for Supported Types:
 * - **Vectors (`vec2`, `vec3`, `vec4`)**: Normalization scales the vector so that its length (magnitude) becomes 1, while preserving its direction.
 * - **Quaternions (`quat`)**: Normalization adjusts the quaternion to unit length, ensuring it represents a valid rotation.
 * - **Euler Angles (`euler3`)**: Normalization wraps each angle within the range [-π, π], ensuring consistency for angle-based calculations, such as rotations.
 *
 * ### Notes:
 * - If the input vector or quaternion has zero length, the function returns a default vector or quaternion with a valid direction (e.g., `(0.0f, 1.0f)` for `vec2`, `(0.0f, 0.0f, 1.0f)` for `vec3`, and `(0.0f, 0.0f, 0.0f, 1.0f)` for `quat`).
 * - The `wrap_angle` function is used to wrap Euler angles into the [-π, π] range, which is useful for applications like smooth rotations and comparisons.
 *
 * ### Supported Types:
 * - `vec2` — 2D vector normalization
 * - `vec3` — 3D vector normalization
 * - `vec4` — 4D vector normalization
 * - `quat` — Quaternion normalization
 * - `euler3` — Euler angle normalization
 */

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/euler3.hpp>

namespace tavros::math
{

    /**
     * @brief Returns a normalized version of the angles.
     *
     * Each angle is normalized into the range [-pi, pi], wrapping values that exceed
     * this range. This is often useful to ensure consistency and prevent issues
     * during interpolation or comparison.
     *
     * @return A new euler3 instance with normalized components.
     */
    euler3 normalize(const euler3& a) noexcept;

    vec2 normalize(const vec2& v) noexcept;

    vec3 normalize(const vec3& v) noexcept;

    vec4 normalize(const vec4& v) noexcept;

    quat normalize(const quat& q) noexcept;

} // namespace tavros::math

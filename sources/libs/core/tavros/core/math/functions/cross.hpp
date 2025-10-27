#pragma once

/**
 * @file cross.hpp
 * @brief Provides functions for computing the cross product in 2D and 3D space.
 *
 * This file defines:
 * - `cross(const vec3&, const vec3&)`: Computes the 3D vector cross product, returning a vector
 *   perpendicular to the plane defined by the two input vectors.
 * - `cross_magnitude(const vec2&, const vec2&)`: Computes the scalar magnitude of the 2D cross product,
 *   which represents the signed area of the parallelogram formed by the two vectors.
 *
 * ### 3D Cross Product:
 * The result is a vector perpendicular to both input vectors.
 *
 * ### 2D Cross Product:
 * The result is a scalar value representing the signed magnitude of the 3D cross product's Z component
 * (assuming the vectors lie in the XY plane).
 *
 * Common use cases:
 * - Geometry and graphics (e.g., calculating normals)
 * - Physics (e.g., torque, angular momentum)
 * - Orientation and rotation operations
 * - Determining winding order or relative orientation in 2D
 *
 * @note If the input vectors are parallel or collinear, the result is zero.
 */

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>

namespace tavros::math
{

    inline constexpr float cross_magnitude(const vec2& a, const vec2& b) noexcept
    {
        return a.x * b.y - a.y * b.x;
    }

    inline constexpr vec3 cross(const vec3& a, const vec3& b) noexcept
    {
        return vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

} // namespace tavros::math

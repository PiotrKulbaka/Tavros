#pragma once

/**
 * @file dot.hpp
 *
 * @brief Dot product of two vectors or quaternions
 *
 * Computes the dot product (scalar product) between two vectors (of any dimension) or two quaternions.
 * It measures how aligned the two objects are in their respective spaces.
 *
 * - Positive result -> vectors/objects point in a similar direction
 * - Zero -> vectors/objects are orthogonal (perpendicular)
 * - Negative result -> vectors/objects point in opposite directions
 *
 * Commonly used in:
 * - Calculating the cosine of the angle between two vectors
 * - Calculating normals for lighting and geometry
 * - Calculating angles between vectors (e.g. for lighting)
 * - Projecting one vector onto another
 * - Determining alignment in physics, shading, and geometry
 *
 * @param a The first vector or quaternion
 * @param b The second vector or quaternion
 * @return Dot product of the two vectors or quaternions
 */

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>
#include <tavros/core/math/quat.hpp>

namespace tavros::math
{

    inline constexpr float dot(const vec2& a, const vec2& b) noexcept
    {
        return a.x * b.x + a.y * b.y;
    }

    inline constexpr float dot(const vec3& a, const vec3& b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline constexpr float dot(const vec4& a, const vec4& b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    inline constexpr float dot(const quat& a, const quat& b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

} // namespace tavros::math

#pragma once

/**
 * @file rotate_point.hpp
 */

namespace tavros::math
{
    class quat;
    class vec3;

    /**
     * @brief Rotates a point using the quaternion.
     * Equivalent to applying the rotation represented by this quaternion to the point.
     */
    vec3 rotate_point(const quat& q, const vec3& p) noexcept;

} // namespace tavros::math

#pragma once

/**
 * @file make_quat.hpp
 */

namespace tavros::math
{
    class euler3;
    class vec3;
    class quat;

    /**
     * @brief Constructs a quaternion from axis-angle rotation
     * @param axis Normalized axis
     * @param angle_rad Rotation angle in radians
     */
    quat make_quat(const vec3& axis, float angle_rad) noexcept;

    /**
     * @brief Constructs a quaternion from euler angles (XYZ order)
     * Assumes left-handed coordinate system (X forward, Y right, Z up)
     */
    quat make_quat(const euler3& euler) noexcept;

    /**
     * @brief Constructs rotation quaternion to rotate from one vector to another.
     * @param from Source direction (normalized).
     * @param to Target direction (normalized).
     */
    quat make_quat(const vec3& from, const vec3& to) noexcept;

    /**
     * @brief Constructs a quaternion looking in the forward direction with a given up vector.
     * Assumes left-handed coordinate system (X forward, Y right, Z up).
     * @param forward Direction to look at (must be normalized).
     * @param up World up direction (must be normalized and not colinear with forward).
     */
    quat make_quat_forward_up(const vec3& forward, const vec3& up) noexcept;

} // namespace tavros::math


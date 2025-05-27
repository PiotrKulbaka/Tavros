#pragma once

/**
 * @file make_mat.hpp
 */

namespace tavros::math
{
    class quat;
    class mat3;
    class mat4;
    class vec3;

    /**
     * @brief Converts quaternion to 3x3 rotation matrix.
     * @note Matrix is column-major (OpenGL-style).
     *       Rotation assumes left-handed coordinate system.
     */
    mat3 make_mat3(const quat& q) noexcept;

    /**
     * @brief Converts quaternion to 4x4 rotation matrix.
     * @note Matrix is column-major (OpenGL-style). Rotation is in upper-left 3x3.
     *       Assumes left-handed coordinate system (X forward, Y right, Z up).
     */
    mat4 make_mat4(const quat& q) noexcept;

    /**
     * @brief Constructs a view matrix oriented in the given forward direction.
     *
     * This function builds a left-handed view matrix, where:
     * - X axis points forward,
     * - Y axis points right,
     * - Z axis points upward.
     *
     * The resulting matrix transforms world-space coordinates into view-space.
     *
     * @param origin The position of the camera in world space.
     * @param forward The normalized forward direction the camera is looking at.
     * @param up The approximate up vector, defining the camera's roll. Must not be collinear with the forward vector.
     * @return A 4x4 matrix representing the view transformation.
     */
    mat4 make_look_at(const vec3& origin, const vec3& forward, const vec3& up) noexcept;

    /**
     * @brief Constructs a perspective projection matrix.
     *
     * This function builds a left-handed perspective projection matrix that maps camera space
     * coordinates to clip space. Designed for use in left-handed coordinate systems.
     *
     * @param fov_y Vertical field of view in radians.
     * @param aspect Aspect ratio (width divided by height).
     * @param z_near Distance to the near clipping plane (must be > 0).
     * @param z_far Distance to the far clipping plane (must be > z_near).
     * @return A 4x4 matrix representing the perspective projection.
     */
    mat4 make_perspective(float fov_y, float aspect, float z_near, float z_far) noexcept;

} // namespace tavros::math

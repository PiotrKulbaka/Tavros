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
     */
    mat3 make_mat3(const quat& q) noexcept;

    /**
     * @brief Converts quaternion to 4x4 rotation matrix.
     * @note Matrix is column-major (OpenGL-style). Rotation is in upper-left 3x3.
     */
    mat4 make_mat4(const quat& q) noexcept;

    /**
     * @brief Constructs a view matrix oriented in the given forward direction.
     *
     * This function builds a right-handed view matrix, where:
     * - X axis points forward,
     * - Y axis points right,
     * - Z axis points upward.
     *
     * The resulting matrix transforms world-space coordinates into view-space.
     *
     * @param origin The position of the camera in world space.
     * @param dir The normalized forward direction the camera is looking at.
     * @param up The approximate up vector, defining the camera's roll. Must not be collinear with the forward vector.
     * @return A 4x4 matrix representing the view transformation.
     */
    mat4 make_look_at_dir(const vec3& origin, const vec3& dir, const vec3& up) noexcept;

    /**
     * @brief Constructs a perspective projection matrix.
     *
     * This function builds a right-handed perspective projection matrix that maps camera space
     * coordinates to clip space.
     *
     * @param fov_y Vertical field of view in radians.
     * @param aspect Aspect ratio (width divided by height).
     * @param z_near Distance to the near clipping plane (must be > 0).
     * @param z_far Distance to the far clipping plane (must be > z_near).
     * @return A 4x4 matrix representing the perspective projection.
     */
    mat4 make_perspective(float fov_y, float aspect, float z_near, float z_far) noexcept;

    /**
     * @brief Constructs an orthographic projection matrix.
     *
     * This function builds a right-handed orthographic projection matrix that maps camera space
     * coordinates to clip space without perspective distortion. All objects retain their size
     * regardless of depth.
     *
     * @param left   Minimum X coordinate of the view volume.
     * @param right  Maximum X coordinate of the view volume.
     * @param bottom Minimum Y coordinate of the view volume.
     * @param top    Maximum Y coordinate of the view volume.
     * @param z_near Distance to the near clipping plane.
     * @param z_far  Distance to the far clipping plane.
     * @return A 4x4 matrix representing the orthographic projection.
     */
    mat4 make_ortho(float left, float right, float bottom, float top, float z_near, float z_far) noexcept;

} // namespace tavros::math

#pragma once

/**
 * @file make_mat.hpp
 */

namespace tavros::math
{
    class quat;
    class mat3;
    class mat4;

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

} // namespace tavros::math

#pragma once

/**
 * @file make_mat.hpp
 */

#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/mat3.hpp>
#include <tavros/core/math/mat4.hpp>

namespace tavros::math
{

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

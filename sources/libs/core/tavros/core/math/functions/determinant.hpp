#pragma once

/**
 * @file determinant.hpp
 * @brief Provides functions to compute the determinant of mat3 and mat4.
 *
 * The determinant is a scalar value that reveals properties of a matrix:
 * - A **zero determinant** indicates that the matrix is **singular** and **not invertible**.
 * - A **non-zero determinant** means the matrix is **invertible** and typically preserves or scales volume.
 *
 * ### Supported Types:
 * - `mat3`
 * - `mat4`
 *
 * ### Use Cases:
 * - Checking if a matrix is invertible before computing its inverse
 * - Calculating volume scaling factors in transformations
 * - Solving systems of linear equations (e.g., Cramer's Rule)
 * - Orientation and handedness checks (e.g., positive or negative determinant)
 *
 * @note Determinant computation can be relatively expensive due to the number of required
 *       multiplications and additions. Use judiciously in performance-critical code.
 *
 * @return A floating-point scalar representing the determinant of the matrix.
 */


#include <tavros/core/math/mat3.hpp>
#include <tavros/core/math/mat4.hpp>

namespace tavros::math
{

    float determinant(const mat3& m) noexcept;

    float determinant(const mat4& m) noexcept;

} // namespace tavros::math

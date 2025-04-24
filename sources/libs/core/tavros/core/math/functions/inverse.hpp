#pragma once

/**
 * @file inverse.hpp
 * @brief Provides functions to compute the inverse of matrices and quaternions.
 *
 * This file defines utility functions to compute the mathematical inverse of:
 * - `mat3` (3x3 matrix)
 * - `mat4` (4x4 matrix)
 * - `quat` (quaternion)
 *
 * Inverse operations are fundamental for reversing transformations or converting
 * between coordinate spaces (e.g., world space to view space).
 *
 * ### Matrix Inversion (`mat3`, `mat4`)
 * - Implemented using the **adjugate** and **determinant** method.
 * - If the determinant is **zero**, the matrix is **singular** and **not invertible**.
 *   In such cases, a **zero matrix** is returned.
 * - It is the caller's responsibility to check invertibility via `determinant()` if needed.
 *
 * ### Quaternion Inversion
 * - Equivalent to the conjugate of the quaternion divided by its squared length.
 * - Assumes normalized quaternions by default; for unnormalized input, inversion still works
 *   but requires additional computation.
 *
 * ### Use Cases:
 * - Reversing transformations
 * - Constructing camera/view matrices
 * - Skinning and animation
 * - Physics simulations
 *
 * @warning For matrices, if the determinant is zero, the inverse is undefined and a zero matrix is returned.
 * @note These operations involve multiple multiplications and additions and may be expensive in performance-critical code.
 *
 * @return The inverted matrix or quaternion.
 * @see determinant(), conjugate()
 */

#include <tavros/core/math/mat3.hpp>
#include <tavros/core/math/mat4.hpp>
#include <tavros/core/math/quat.hpp>

namespace tavros::math
{

    mat3 inverse(const mat3& m) noexcept;

    mat4 inverse(const mat4& m) noexcept;

    quat inverse(const quat& q) noexcept;

} // namespace tavros::math

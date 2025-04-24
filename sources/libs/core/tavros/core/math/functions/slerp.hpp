#pragma once

/**
 * @file slerp.hpp
 * @brief Provides spherical linear interpolation between two quaternions.
 *
 * The interpolation coefficient can be in range [0..1], but can be outside this range,
 * not clamped
 *
 * Supported types:
 * - `quat`
 */

#include <tavros/core/math/quat.hpp>

namespace tavros::math
{

    quat slerp(const quat& a, const quat& b, float coef) noexcept;

} // namespace tavros::math

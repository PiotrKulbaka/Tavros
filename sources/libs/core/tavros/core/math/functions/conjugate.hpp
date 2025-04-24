#pragma once

/**
 * @file conjugate.hpp
 * @brief Contains a function that returns the conjugate of a quaternion.
 *
 * The conjugate of a quaternion is computed by negating the vector part (x, y, z)
 * and keeping the scalar part (w) unchanged. It is commonly used to compute
 * the inverse of a unit quaternion or to undo a rotation.
 */

#include <tavros/core/math/quat.hpp>

namespace tavros::math
{

    quat conjugate(const quat& q) noexcept
    {
        return quat(-q.x, -q.y, -q.z, q.w);
    }

} // namespace tavros::math

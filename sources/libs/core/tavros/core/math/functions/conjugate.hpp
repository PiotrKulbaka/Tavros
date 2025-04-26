#pragma once

/**
 * @file conjugate.hpp
 */

namespace tavros::math
{
    class quat;

    /**
     * @brief Returns the conjugate of a quaternion.
     *
     * The conjugate of a quaternion is computed by negating the vector part (x, y, z)
     * and keeping the scalar part (w) unchanged. It is commonly used to compute
     * the inverse of a unit quaternion or to undo a rotation.
     */
    quat conjugate(const quat& q) noexcept;

} // namespace tavros::math

#pragma once

/**
 * @file make_euler.hpp
 */

namespace tavros::math
{
    class quat;
    class euler3;

    /**
     * @brief Converts quaternion to euler angles in radians (XYZ rotation order).
     * Assumes left-handed coordinate system (X forward, Y right, Z up).
     */
    euler3 make_euler3(const quat& q) noexcept;

} // namespace tavros::math

#pragma once

/**
 * @file make_euler.hpp
 */

#include <tavros/core/math/functions/basic_math.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/euler3.hpp>

namespace tavros::math
{

    /**
     * @brief Converts quaternion to euler angles in radians (XYZ rotation order).
     * Assumes left-handed coordinate system (X forward, Y right, Z up).
     */
    euler3 make_euler3(const quat& q) noexcept;

} // namespace tavros::math

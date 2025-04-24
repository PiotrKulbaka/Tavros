#pragma once

/**
 * @file rotate_point.hpp
 */

#include <tavros/core/math/functions/inverse.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/vec3.hpp>

namespace tavros::math
{
    /**
     * @brief Rotates a point using the quaternion.
     * Equivalent to applying the rotation represented by this quaternion to the point.
     * Assumes left-handed coordinate system.
     */
    vec3 rotate_point(const quat& q, const vec3& p) noexcept
    {
        auto inv = inverse(q);
        auto qp = quat(p.x, p.y, p.z, 0);
        auto res = q * qp * inv;
        return vec3(res.x, res.y, res.z);
    }

} // namespace tavros::math

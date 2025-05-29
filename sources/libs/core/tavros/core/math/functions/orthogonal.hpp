#pragma once

/**
 * @file orthogonal.hpp
 */

#include <tavros/core/math/vec3.hpp>

namespace tavros::math
{

    /**
     * @brief Returns an arbitrary vector orthogonal to this one.
     *
     * The result is not normalized. The smallest component is selected to minimize precision loss.
     * If this vector is zero, the result is undefined.
     *
     * @return A vector perpendicular to this one.
     */
    inline vec3 orthogonal(const vec3& v) noexcept
    {
        if (abs(v.x) < abs(v.y)) {
            if (abs(v.x) < abs(v.z)) {
                // X minimal
                return vec3(0.0f, -v.z, v.y);
            } else {
                // Z minimal
                return vec3(-v.y, v.x, 0.0f);
            }
        } else {
            if (abs(v.y) < abs(v.z)) {
                // Y minimal
                return vec3(-v.z, 0.0f, v.x);
            } else {
                // Z minimal
                return vec3(-v.y, v.x, 0.0f);
            }
        }
    }

} // namespace tavros::math

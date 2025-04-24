#pragma once

#include <tavros/core/math/functions/length.hpp>
#include <tavros/core/math/functions/dot.hpp>
#include <tavros/core/math/vec3.hpp>

namespace tavros::math
{

    /**
     * @brief Calculates the angle (in radians) between this vector and another vector.
     *
     * The angle is computed using the formula:
     *      θ = acos((A · B) / (|A| * |B|))
     *
     * @note In the case where either of the vectors has zero length, the angle
     *       is defined as 0 radians (k_epsilon6 is used for comparisons)
     *
     * @note In debug, an assertion will be triggered if either of the vectors has zero length.
     *
     * @param other The other vector to compare this vector with.
     * @return The angle between the two vectors in radians.
     */
    float angle_between(const vec3& a, const vec3& b) noexcept;

} // namespace tavros::math


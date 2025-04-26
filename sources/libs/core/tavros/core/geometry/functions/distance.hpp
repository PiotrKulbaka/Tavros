#pragma once

/**
 * @file distance.hpp
 */

#include <tavros/core/math.hpp>

namespace tavros::geometry
{
    class plane;

    /**
     * @brief Computes the signed distance from a point to the plane.
     *
     * @param point The point to evaluate.
     * @return The signed distance. Positive if the point lies in the direction of the normal, negative if opposite.
     */
    float distance(const plane& plane, const math::vec3& point) noexcept;

} // namespace tavros::geometry


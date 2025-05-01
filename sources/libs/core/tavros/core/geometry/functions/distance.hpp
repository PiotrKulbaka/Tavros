#pragma once

/**
 * @file distance.hpp
 */

#include <tavros/core/math.hpp>

namespace tavros::geometry
{
    class plane;
    class sphere;

    /**
     * @brief Computes the signed distance from a point to the plane.
     *
     * @param point The point to evaluate.
     * @return The signed distance. Positive if the point lies in the direction of the normal, negative if opposite.
     */
    float distance(const plane& plane, const math::vec3& point) noexcept;

    /**
     * @brief Returns the actual (non-squared) distance from the point to the sphere surface.
     *
     * The returned distance can be negative if the point is inside the sphere.
     * A negative distance indicates that the point lies within the sphere.
     * A distance of zero indicates that the point lies on the surface of the sphere.
     * A positive distance indicates that the point is outside the sphere.
     *
     * @param sphere The sphere to test
     * @param point The point to test
     * @return The signed distance from the point to the sphere surface
     */
    float distance(const sphere& sphere, const math::vec3& point) noexcept;

} // namespace tavros::geometry


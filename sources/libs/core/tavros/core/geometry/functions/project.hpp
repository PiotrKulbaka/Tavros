#pragma once

/**
 * @file project.hpp
 */

#include <tavros/core/math.hpp>

namespace tavros::geometry
{
    class plane;
    class sphere;

    /**
     * @brief Projects a point orthogonally onto the plane
     *
     * @param point The point to project
     * @param plane The plane onto which to project the point
     * @return The nearest point on the plane
     */
    math::vec3 project_point(const math::vec3& point, const plane& plane) noexcept;

    /**
     * @brief Projects a point orthogonally onto the surface of the sphere
     *
     * The method projects the input point onto the surface of the sphere, ensuring the projected point
     * lies exactly on the sphere's surface. If the point is inside the sphere, it is projected
     * onto the nearest point on the surface. If the point is at the center of the sphere,
     * the projection is along the positive Z axis
     *
     * @param point The point to project onto the sphere
     * @param sphere The sphere onto which to project the point
     * @return The projected point on the sphere's surface
     */
    math::vec3 project_point(const math::vec3& point, const sphere& sphere) noexcept;

} // namespace tavros::geometry


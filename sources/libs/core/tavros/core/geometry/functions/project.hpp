#pragma once

/**
 * @file project.hpp
 */

#include <tavros/core/math.hpp>

namespace tavros::geometry
{
    class plane;

    /**
     * @brief Projects a point orthogonally onto the plane.
     *
     * @param point The point to project.
     * @return The nearest point on the plane.
     */
    math::vec3 project_point(const plane& plane, const math::vec3& point) noexcept;

} // namespace tavros::geometry


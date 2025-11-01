#pragma once

#include <tavros/core/math/vec3.hpp>

namespace tavros::geometry
{
    /**
     * @brief Oriented Bounding Box (OBB) in 3D space.
     *
     * The `obb3` class represents an **Oriented Bounding Box**, a box-shaped volume in 3D space
     * that can be arbitrarily rotated and translated. Unlike Axis-Aligned Bounding Boxes (AABB),
     * which are always aligned to the global coordinate axes, an OBB can have any orientation,
     * making it more accurate for tight fitting around rotated or non-uniformly shaped objects.
     *
     * This class is commonly used in:
     * - Collision detection
     * - Visibility testing and frustum culling
     * - Spatial partitioning
     * - Physics simulations and bounding volume hierarchies
     *
     * The AABB can be used for tasks such as:
     * - Constructing an OBB from transformation matrices or AABB
     * - Checking point or box containment
     * - Intersecting with rays or other bounding volumes
     * - Transforming the OBB by a matrix or set of transforms
     */
    class obb3
    {
    public:
        constexpr obb3() noexcept = default;

        constexpr obb3(const math::vec3& center, const math::vec3& forward, const math::vec3& right, const math::vec3& up, const math::vec3& half_extents) noexcept
            : center(center)
            , forward(forward)
            , right(right)
            , up(up)
            , half_extents(half_extents)
        {
        }

        /**
         * @brief Checks if a point is inside the OBB.
         *
         * @param point The point to test.
         * @return true if the point is inside the OBB, false otherwise.
         */
        bool contains_point(const math::vec3& point) const noexcept;

    public:
        math::vec3 center; // Center of the OBB
        math::vec3 forward;
        math::vec3 right;
        math::vec3 up;
        math::vec3 half_extents; // Half-extents along each of the axes
    };

} // namespace tavros::geometry

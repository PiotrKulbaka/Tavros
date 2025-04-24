#pragma once

#include <tavros/core/math.hpp>

namespace tavros::geometry
{
    /**
     * @brief Represents a sphere in 3D space.
     *
     * The sphere is defined by its center and radius.
     */
    class sphere
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes a sphere with center at the origin and zero radius.
         */
        constexpr sphere() noexcept;

        /**
         * @brief Constructs a sphere from a center point and a radius.
         *
         * @param center Center of the sphere.
         * @param radius Radius of the sphere.
         */
        constexpr sphere(const math::vec3& center, float radius) noexcept;

        /**
         * @brief Returns the actual (non-squared) distance from the point to the sphere surface.
         *
         * The returned distance can be negative if the point is inside the sphere.
         * A negative distance indicates that the point lies within the sphere.
         * A distance of zero indicates that the point lies on the surface of the sphere.
         * A positive distance indicates that the point is outside the sphere.
         *
         * @param point The point to test.
         * @return The signed distance from the point to the sphere surface.
         */
        float distance(const math::vec3& point) const noexcept;

        /**
         * @brief Returns the squared distance from the point to the sphere surface.
         *
         * The returned squared distance will be negative if the point is inside the sphere.
         * A squared distance of zero indicates that the point lies on the surface of the sphere.
         * A positive squared distance indicates that the point is outside the sphere.
         *
         * @param point The point to test.
         * @return The squared distance from the point to the sphere surface.
         */
        float squared_distance(const math::vec3& point) const noexcept;

        /**
         * @brief Projects a point orthogonally onto the surface of the sphere.
         *
         * The method projects the input point onto the surface of the sphere, ensuring the projected point
         * lies exactly on the sphere's surface. If the point is inside the sphere, it is projected
         * onto the nearest point on the surface. If the point is at the center of the sphere,
         * the projection is along the positive Z axis.
         *
         * @param point The point to project onto the sphere.
         * @return The projected point on the sphere's surface.
         */
        math::vec3 project_point(const math::vec3& point) const noexcept;

    public:
        math::vec3 center;
        float      radius;
    };

    static_assert(sizeof(sphere) == 16, "incorrect size");
    static_assert(alignof(sphere) == 4, "incorrect alignment");

    inline constexpr sphere::sphere() noexcept
        : center(0.0f, 0.0f, 0.0f)
        , radius(0.0f)
    {
    }

    inline constexpr sphere::sphere(const math::vec3& center, float radius) noexcept
        : center(center)
        , radius(radius)
    {
    }

} // namespace tavros::geometry

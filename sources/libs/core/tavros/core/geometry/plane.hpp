#pragma once

#include <tavros/core/math.hpp>

namespace tavros::geometry
{

    /**
     * @brief Represents a plane in 3D space.
     *
     * The plane is defined by a normal vector and a scalar distance from the origin.
     * The implicit equation of the plane is: dot(normal, point) + d = 0
     *
     * By default, the plane lies in the XY plane (normal is (0, 0, 1)), meaning it faces
     * along the positive Z axis.
     */
    class plane
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the plane with normal (0, 0, 1) and d = 0.
         * This creates a plane lying along the X and Y axes at Z = 0.
         */
        constexpr plane() noexcept;

        /**
         * @brief Constructs a plane from a normal and a distance.
         *
         * @param normal A normalized vector representing the plane's normal.
         * @param d The signed distance from the origin to the plane along the normal.
         *
         * The plane equation is: dot(normal, point) + d = 0.
         *
         * @note The value of `d` should typically be negative, as it is calculated as -dot(normal, point_on_plane).
         * Providing a positive `d` positions the plane in the direction opposite to the one given by -dot(normal, point).
         */
        constexpr plane(const math::vec3& normal, float d) noexcept;

        /**
         * @brief Constructs a plane from a normal and a point on the plane.
         *
         * @param normal A normalized vector representing the plane's normal.
         * @param point A point lying on the plane.
         */
        constexpr plane(const math::vec3& normal, const math::vec3& point) noexcept;

        /**
         * @brief Constructs a plane from three non-collinear points.
         *
         * @param a First point.
         * @param b Second point.
         * @param c Third point.
         *
         * The normal is computed from the cross product of vectors (b - a) and (c - a).
         */
        plane(const math::vec3& a, const math::vec3& b, const math::vec3& c) noexcept;

        /**
         * @brief Computes the signed distance from a point to the plane.
         *
         * @param point The point to evaluate.
         * @return The signed distance. Positive if the point lies in the direction of the normal, negative if opposite.
         */
        float distance(const math::vec3& point) const noexcept;

        /**
         * @brief Projects a point orthogonally onto the plane.
         *
         * @param point The point to project.
         * @return The nearest point on the plane.
         */
        math::vec3 project_point(const math::vec3& point) const noexcept;

    public:
        math::vec3 normal; // The normalized normal vector of the plane
        float      d;      // The signed distance from the origin to the plane along the normal
                           // (used in plane equation: dot(normal, point) + d = 0)
    };

    static_assert(sizeof(plane) == 16, "incorrect size");
    static_assert(alignof(plane) == 4, "incorrect alignment");

    inline constexpr plane::plane() noexcept
        : normal(0.0, 0.0, 1.0)
        , d(0.0)
    {
    }

    inline constexpr plane::plane(const math::vec3& normal, float d) noexcept
        : normal(normal)
        , d(d)
    {
    }

    inline constexpr plane::plane(const math::vec3& normal, const math::vec3& point) noexcept
        : normal(normal)
        , d(-math::dot(normal, point))
    {
    }

} // namespace tavros::geometry

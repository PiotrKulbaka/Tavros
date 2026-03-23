#pragma once
#include <tavros/core/math.hpp>

namespace tavros::geometry
{
    /**
     * @brief Capsule in 3D space.
     *
     * The `capsule` class represents a **Capsule** - a cylinder with hemispherical caps on both ends,
     * defined by two endpoint centers and a radius. It is one of the most commonly used primitive
     * shapes in physics engines and collision detection due to its mathematical simplicity and
     * smooth surface properties.
     *
     * A capsule is defined by:
     * - Two endpoint centers (`a` and `b`) representing the centers of the hemispherical caps
     * - A `radius` defining the thickness
     *
     * The capsule is commonly used in:
     * - Character controller collision volumes
     * - Physics simulations (ragdolls, projectiles)
     * - Swept sphere tests and continuous collision detection
     * - Bounding volume hierarchies
     *
     * The capsule can be used for tasks such as:
     * - Checking point containment
     * - Intersecting with rays, spheres, planes and other volumes
     * - Computing the closest point on the capsule axis to a given point
     * - Transforming the capsule by a matrix or set of transforms
     */
    class capsule
    {
    public:
        constexpr capsule() noexcept = default;

        constexpr capsule(const math::vec3& a, const math::vec3& b, float radius) noexcept
            : a(a)
            , b(b)
            , radius(radius)
        {
        }

        /**
         * @brief Returns the center point of the capsule (midpoint of the axis segment).
         *
         * @return The midpoint between `a` and `b`.
         */
        [[nodiscard]] constexpr math::vec3 center() const noexcept
        {
            return (a + b) * 0.5f;
        }

        /**
         * @brief Returns the length of the inner axis segment (excluding hemispheres).
         *
         * @return Distance between `a` and `b`.
         */
        [[nodiscard]] float height() const noexcept
        {
            return math::length(b - a);
        }

        /**
         * @brief Returns the total height of the capsule including both hemispherical caps.
         *
         * @return Distance between `a` and `b` plus two radii.
         */
        [[nodiscard]] float full_height() const noexcept
        {
            return math::length(b - a) + 2.0f * radius;
        }

        /**
         * @brief Returns the normalized direction vector along the capsule axis from `a` to `b`.
         *
         * @return Unit vector from `a` to `b`, or zero vector if `a == b`.
         */
        [[nodiscard]] math::vec3 axis_direction() const noexcept
        {
            auto  d = b - a;
            float len = math::length(d);
            if (len < math::k_epsilon6) {
                return math::vec3(0.0f, 0.0f, 1.0f);
            }
            return d / len;
        }

        /**
         * @brief Checks if a point is inside the capsule.
         *
         * @param point The point to test.
         * @return true if the point is inside or on the surface of the capsule.
         */
        [[nodiscard]] bool contains_point(const math::vec3& point) const noexcept
        {
            auto  closest = closest_point_on_axis(point);
            float dist_sq = math::dot(point - closest, point - closest);
            return dist_sq <= radius * radius;
        }

        /**
         * @brief Returns the closest point on the capsule's inner axis segment to a given point.
         *
         * @param point The reference point.
         * @return The closest point on the segment [a, b].
         */
        [[nodiscard]] math::vec3 closest_point_on_axis(const math::vec3& point) const noexcept
        {
            auto  ab = b - a;
            float len_sq = math::dot(ab, ab);
            if (len_sq < 1e-12f) {
                return a;
            }
            float t = math::dot(point - a, ab) / len_sq;
            t = math::clamp(t, 0.0f, 1.0f);
            return a + ab * t;
        }

    public:
        math::vec3 a;
        math::vec3 b;
        float      radius = 0.0f;
    };

} // namespace tavros::geometry

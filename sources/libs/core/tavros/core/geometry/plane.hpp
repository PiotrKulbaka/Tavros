#pragma once
#include <tavros/core/math.hpp>

namespace tavros::geometry
{
    /**
     * @brief Plane in 3D space defined by a normal and a signed distance from the origin.
     *
     * Implicit equation: dot(normal, point) + distance = 0
     * The normal points to the "positive" side of the plane.
     * By default, the plane lies in the XY plane with normal (0, 0, 1).
     */
    class plane
    {
    public:
        constexpr plane() noexcept = default;

        constexpr plane(const math::vec3& normal, float distance) noexcept
            : normal(normal)
            , distance(distance)
        {
        }

        /**
         * @brief Constructs a plane from a normal and a point on the plane.
         */
        static constexpr plane from_normal_point(const math::vec3& normal, const math::vec3& point) noexcept
        {
            return plane(normal, -math::dot(normal, point));
        }

        /**
         * @brief Constructs a plane from three non-collinear points (counter-clockwise winding).
         */
        static plane from_points(const math::vec3& a, const math::vec3& b, const math::vec3& c) noexcept
        {
            auto n = math::normalize(math::cross(c - a, b - a));
            return plane(n, -math::dot(n, a));
        }

        /**
         * @brief Returns the signed distance from a point to the plane.
         *
         * Positive if the point is on the side the normal points to, negative otherwise.
         */
        [[nodiscard]] constexpr float signed_distance(const math::vec3& point) const noexcept
        {
            return math::dot(normal, point) + distance;
        }

        /**
         * @brief Returns the closest point on the plane to a given point.
         */
        [[nodiscard]] constexpr math::vec3 closest_point(const math::vec3& point) const noexcept
        {
            return point - normal * signed_distance(point);
        }

        /**
         * @brief Checks if a point is on the positive side of the plane.
         */
        [[nodiscard]] constexpr bool is_on_positive_side(const math::vec3& point) const noexcept
        {
            return signed_distance(point) >= 0.0f;
        }

        /**
         * @brief Returns a plane with the same position but flipped normal.
         */
        [[nodiscard]] constexpr plane flipped() const noexcept
        {
            return plane(-normal, -distance);
        }

        /**
         * @brief Returns a normalized version of the plane.
         *
         * Use when the normal may not be unit-length (e.g. after extracting from a matrix).
         */
        [[nodiscard]] plane normalized() const noexcept
        {
            float len = math::length(normal);
            return plane(normal / len, distance / len);
        }

    public:
        math::vec3 normal = math::vec3(0.0f, 0.0f, 1.0f);
        float      distance = 0.0f;
    };

    static_assert(sizeof(plane) == 16, "incorrect size");
    static_assert(alignof(plane) == 4, "incorrect alignment");

} // namespace tavros::geometry

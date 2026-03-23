#pragma once

#include <tavros/core/math.hpp>

namespace tavros::geometry
{

    /**
     * @brief Represents a ray in 3D space.
     *
     * A ray is defined by an origin point and a normalized direction vector.
     */
    class ray3
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Creates a ray starting at the origin (0, 0, 0) pointing along positive Z axis (0, 0, 1).
         */
        constexpr ray3() noexcept
            : origin(0.0f, 0.0f, 0.0f)
            , direction(0.0f, 0.0f, 1.0f)
        {
        }

        /**
         * @brief Constructs a ray with given origin and direction.
         *
         * @param origin The starting point of the ray
         * @param direction The direction of the ray (should be normalized)
         */
        constexpr ray3(const math::vec3& origin, const math::vec3& direction) noexcept
            : origin(origin)
            , direction(direction)
        {
        }

        /**
         * @brief Computes the point at a given distance along the ray
         *
         * @param distance The distance along the ray
         * @return The point at the given distance along the ray
         */
        constexpr math::vec3 at(float distance) const noexcept
        {
            return origin + direction * distance;
        }

    public:
        math::vec3 origin;
        math::vec3 direction;
    };

    static_assert(sizeof(ray3) == 24, "incorrect size");
    static_assert(alignof(ray3) == 4, "incorrect alignment");

} // namespace tavros::geometry

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
        constexpr ray3() noexcept;

        /**
         * @brief Constructs a ray with given origin and direction.
         *
         * @param origin The starting point of the ray
         * @param direction The direction of the ray (should be normalized)
         */
        constexpr ray3(const math::vec3& origin, const math::vec3& direction) noexcept;

        /**
         * @brief Computes the point at a given distance along the ray
         *
         * @param distance The distance along the ray
         * @return The point at the given distance along the ray
         */
        constexpr math::vec3 at(float distance) const noexcept;

    public:
        math::vec3 origin;
        math::vec3 direction;
    };

} // namespace tavros::geometry

#include <tavros/core/geometry/ray3.inl>

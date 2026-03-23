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
        constexpr sphere() noexcept = default;

        /**
         * @brief Constructs a sphere from a center point and a radius.
         *
         * @param center Center of the sphere.
         * @param radius Radius of the sphere.
         */
        constexpr sphere(const math::vec3& center, float radius) noexcept
            : center(center)
            , radius(radius)
        {
        }

    public:
        math::vec3 center;
        float      radius = 0.0f;
    };

    static_assert(sizeof(sphere) == 16, "incorrect size");
    static_assert(alignof(sphere) == 4, "incorrect alignment");

} // namespace tavros::geometry

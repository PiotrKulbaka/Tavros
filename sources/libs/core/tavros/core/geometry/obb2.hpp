#pragma once

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/mat3.hpp>

namespace tavros::geometry
{
    /**
     * @brief Oriented Bounding Box (OBB) in 2D space.
     */
    class obb2
    {
    public:
        constexpr obb2() noexcept = default;

        obb2(const math::vec2& center, const math::vec2& right, const math::vec2& up, const math::vec2& half_extents) noexcept
            : center(center)
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
        bool contains_point(const math::vec2& point) const noexcept;

        /**
         * @brief Converts the OBB to a transformation matrix.
         *
         * Builds a 3x3 matrix that represents the oriented bounding box in world space.
         *
         * @return A 3x3 transformation matrix representing this OBB.
         */
        math::mat3 to_mat() const noexcept
        {
            return math::mat3(
                math::vec3(right * half_extents.x),
                math::vec3(up * half_extents.y),
                math::vec3(center, 1.0f)
            );
        }

    public:
        math::vec2 center; // Center of the OBB
        math::vec2 right;
        math::vec2 up;
        math::vec2 half_extents; // Half-extents along each of the axes
    };

} // namespace tavros::geometry

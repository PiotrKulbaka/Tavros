#pragma once

#include <tavros/core/math/vec2.hpp>

    namespace tavros::core::math
{
    /**
     * @brief Axis-aligned bounding box in 2D space.
     */
    class aabb2
    {
    public:
        /**
         * @brief Constructs an invalid AABB (min > max).
         */
        aabb2() noexcept;

        /**
         * @brief Constructs AABB from two corner points.
         * @param min_point Lower-left corner.
         * @param max_point Upper-right corner.
         */
        aabb2(const vec2& min_point, const vec2& max_point) noexcept;

        /**
         * @brief Returns the center point of the AABB.
         */
        vec2 center() const noexcept;

        /**
         * @brief Returns the size (width and height) of the AABB.
         */
        vec2 size() const noexcept;

        /**
         * @brief Returns the area of the AABB.
         */
        float area() const noexcept;

        /**
         * @brief Checks if the AABB contains a point.
         * @param point Point to check.
         */
        bool contains_point(const vec2& point) const noexcept;

        /**
         * @brief Checks if this AABB intersects with another AABB.
         * @param other AABB to check against.
         */
        bool intersects(const aabb2& other) const noexcept;

        /**
         * @brief Expands the AABB to include a point.
         * @param point Point to include.
         */
        void expand(const vec2& point) noexcept;

        /**
         * @brief Returns a new AABB that merges this and another AABB.
         * @param other AABB to merge with.
         */
        aabb2 merged(const aabb2& other) const noexcept;

        /**
         * @brief Expands this AABB to include another AABB.
         * @param other AABB to include.
         */
        void merge(const aabb2& other) noexcept;

        /**
         * @brief Resets the AABB to an invalid state.
         */
        void reset() noexcept;

        /**
         * @brief Returns distance from a point to the AABB (0 if inside).
         * @param point Point to check.
         */
        float distance(const vec2& point) const noexcept;

    public:
        vec2 min;
        vec2 max;
    };

} // namespace tavros::core::math

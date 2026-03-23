#pragma once

#include <tavros/core/math.hpp>
#include <limits>

namespace tavros::geometry
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
        aabb2() noexcept
            : min(std::numeric_limits<float>::max())
            , max(std::numeric_limits<float>::lowest())
        {
        }

        /**
         * @brief Constructs AABB from two corner points.
         * @param min Lower-left corner.
         * @param max Upper-right corner.
         */
        aabb2(const math::vec2& min, const math::vec2& max) noexcept
            : min(min)
            , max(max)
        {
        }

        /**
         * @brief Constructs AABB from two corner points.
         * @param left side.
         * @param top side.
         * @param right side.
         * @param bottom side.
         */
        aabb2(float left, float top, float right, float bottom) noexcept
            : min(left, top)
            , max(right, bottom)
        {
        }

        /**
         * @brief Returns the center point of the AABB.
         */
        math::vec2 center() const noexcept
        {
            return (min + max) * 0.5f;
        }

        /**
         * @brief Returns the size (width and height) of the AABB.
         */
        math::vec2 size() const noexcept
        {
            return max - min;
        }

        /**
         * @brief Returns the width of the AABB.
         */
        float width() const noexcept
        {
            return max.x - min.x;
        }

        /**
         * @brief Returns the height of the AABB.
         */
        float height() const noexcept
        {
            return max.y - min.y;
        }

        /**
         * @brief Returns the area of the AABB.
         */
        float area() const noexcept
        {
            const math::vec2 s = size();
            return s.x * s.y;
        }

        /**
         * @brief Checks if the AABB contains a point.
         * @param point Point to check.
         */
        bool contains_point(const math::vec2& point) const noexcept
        {
            return (point.x >= min.x && point.y >= min.y)
                && (point.x <= max.x && point.y <= max.y);
        }

        /**
         * @brief Expands the AABB to include a point.
         * @param point Point to include.
         */
        void expand(const math::vec2& point) noexcept
        {
            min = tavros::math::min(min, point);
            max = tavros::math::max(max, point);
        }

        /**
         * @brief Returns a new AABB that merges this and another AABB.
         * @param other AABB to merge with.
         */
        aabb2 merged(const aabb2& other) const noexcept
        {
            return aabb2(tavros::math::min(min, other.min), tavros::math::max(max, other.max));
        }

        /**
         * @brief Expands this AABB to include another AABB.
         * @param other AABB to include.
         */
        void merge(const aabb2& other) noexcept
        {
            min = tavros::math::min(min, other.min);
            max = tavros::math::max(max, other.max);
        }

        /**
         * @brief Resets the AABB to an invalid state.
         */
        void reset() noexcept
        {
            min = math::vec2(std::numeric_limits<float>::max());
            max = math::vec2(std::numeric_limits<float>::lowest());
        }

        /**
         * @brief Returns distance from a point to the AABB (0 if inside).
         * @param point Point to check.
         */
        float distance(const math::vec2& point) const noexcept
        {
            const float dx = tavros::math::max(min.x - point.x, 0.0f, point.x - max.x);
            const float dy = tavros::math::max(min.y - point.y, 0.0f, point.y - max.y);
            return std::sqrt(dx * dx + dy * dy);
        }

    public:
        union
        {
            struct
            {
                float left;
                float top;
                float right;
                float bottom;
            };
            struct
            {
                math::vec2 min;
                math::vec2 max;
            };
        };
    };

    static_assert(sizeof(aabb2) == 16, "incorrect size");
    static_assert(alignof(aabb2) == 4, "incorrect alignment");

} // namespace tavros::geometry

#pragma once

#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/mat4.hpp>

namespace tavros::geometry
{

    /**
     * @brief Axis-aligned bounding box in 2D space.
     */
    class aabb3
    {
    public:
        /**
         * @brief Constructs an invalid AABB (min > max).
         */
        aabb3() noexcept;

        /**
         * @brief Constructs an AABB from min and max points.
         * @param min_point Minimum corner of the box.
         * @param max_point Maximum corner of the box.
         */
        aabb3(const math::vec3& min_point, const math::vec3& max_point) noexcept;

        /**
         * @brief Returns the center point of the box.
         */
        math::vec3 center() const noexcept;

        /**
         * @brief Returns the size of the box (max - min).
         */
        math::vec3 size() const noexcept;

        /**
         * @brief Returns the volume of the box.
         */
        float volume() const noexcept;

        /**
         * @brief Returns true if the box contains the given point.
         * @param point Point to test.
         */
        bool contains_point(const math::vec3& point) const noexcept;

        /**
         * @brief Returns true if this AABB intersects with another.
         * @param other The other bounding box.
         */
        bool intersects(const aabb3& other) const noexcept;

        /**
         * @brief Expands the AABB to include the given point.
         * @param point Point to include.
         */
        void expand(const math::vec3& point) noexcept;

        /**
         * @brief Returns a new AABB that merges this box with another.
         * @param other Box to merge with.
         */
        aabb3 merged(const aabb3& other) const noexcept;

        /**
         * @brief Merges this box with another AABB in place.
         * @param other Box to merge with.
         */
        void merge(const aabb3& other) noexcept;

        /**
         * @brief Resets the box to an invalid state (min > max).
         */
        void reset() noexcept;

        /**
         * @brief Returns distance from a point to the AABB (0 if inside).
         * @param point Point to test.
         */
        float distance(const math::vec3& point) const noexcept;

        /**
         * @brief Returns a new AABB that bounds this one transformed by a matrix.
         * @param transform Transformation matrix.
         */
        aabb3 transformed(const math::mat4& transform) const noexcept;

    public:
        math::vec3 min;
        math::vec3 max;
    };

} // namespace tavros::geometry

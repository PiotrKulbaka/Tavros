#pragma once

#include <tavros/core/math.hpp>
#include <limits>

namespace tavros::geometry
{

    /**
     * @brief Axis-Aligned Bounding Box in 3D space.
     *
     * This class represents a 3D axis-aligned bounding box (AABB). An AABB is a box
     * whose faces are aligned with the coordinate axes, meaning that its sides are
     * parallel to the X, Y, and Z axes.
     *
     * The AABB is defined by two points:
     * - The minimum point (min) which represents the corner with the smallest coordinates.
     * - The maximum point (max) which represents the corner with the largest coordinates.
     *
     * The AABB is typically used in collision detection and spatial partitioning,
     * as it provides a simple and efficient way to approximate the size and position
     * of objects in 3D space.
     *
     * The AABB is also commonly used in frustum culling, bounding volume hierarchies,
     * and ray tracing algorithms.
     *
     * @note The AABB does not support rotations. If an object is rotated, its bounding
     *       volume should be recalculated, often with the help of a more complex bounding
     *       shape like an oriented bounding box (OBB).
     *
     * The AABB can be used for tasks such as:
     * - Checking if an object is inside another (e.g., if two AABBs overlap).
     * - Quickly testing if a point lies within the bounds of the box.
     * - Testing ray intersection with a box.
     *
     * @note The AABB is considered "empty" if the min is greater than the max component-wise.
     */
    class aabb3
    {
    public:
        /**
         * @brief Constructs an invalid AABB (min > max).
         */
        aabb3() noexcept
            : min(std::numeric_limits<float>::max())
            , max(std::numeric_limits<float>::lowest())
        {
        }

        /**
         * @brief Constructs an AABB from min and max points.
         * @param min Minimum corner of the box.
         * @param max Maximum corner of the box.
         */
        aabb3(const math::vec3& min, const math::vec3& max) noexcept
            : min(min)
            , max(max)
        {
        }

        /**
         * @brief Returns the center point of the box.
         */
        math::vec3 center() const noexcept
        {
            return (min + max) * 0.5f;
        }

        /**
         * @brief Returns the size of the box (max - min).
         */
        math::vec3 size() const noexcept
        {
            return max - min;
        }

        /**
         * @brief Returns the volume of the box.
         */
        float volume() const noexcept
        {
            const auto s = size();
            return s.x * s.y * s.z;
        }

        /**
         * @brief Returns true if the box contains the given point.
         * @param point Point to test.
         */
        bool contains_point(const math::vec3& point) const noexcept
        {
            return (point.x >= min.x && point.x <= max.x)
                && (point.y >= min.y && point.y <= max.y)
                && (point.z >= min.z && point.z <= max.z);
        }

        /**
         * @brief Expands the AABB to include the given point.
         * @param point Point to include.
         */
        void expand(const math::vec3& point) noexcept
        {
            min = math::min(this->min, point);
            max = math::max(this->max, point);
        }

        /**
         * @brief Returns a new AABB that merges this box with another.
         * @param other Box to merge with.
         */
        aabb3 merged(const aabb3& other) const noexcept
        {
            return aabb3(math::min(min, other.min), math::max(max, other.max));
        }

        /**
         * @brief Merges this box with another AABB in place.
         * @param other Box to merge with.
         */
        void merge(const aabb3& other) noexcept
        {
            min = math::min(this->min, other.min);
            max = math::max(this->max, other.max);
        }

        /**
         * @brief Resets the box to an invalid state (min > max).
         */
        void reset() noexcept
        {
            min = math::vec3(std::numeric_limits<float>::max());
            max = math::vec3(std::numeric_limits<float>::lowest());
        }

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

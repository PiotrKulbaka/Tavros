#pragma once
#include <tavros/core/math.hpp>
#include <tavros/core/geometry/plane.hpp>
#include <tavros/core/geometry/sphere.hpp>
#include <tavros/core/geometry/aabb3.hpp>

namespace tavros::geometry
{
    /**
     * @brief View frustum in 3D space.
     *
     * The `frustum` class represents a **View Frustum** - a truncated pyramid defined by six
     * clipping planes (near, far, left, right, top, bottom). It is typically constructed from
     * a combined view-projection matrix and used to determine which objects are visible to a camera.
     *
     * The frustum is commonly used in:
     * - Frustum culling (skipping rendering of off-screen objects)
     * - Visibility testing in scene graphs and BVH traversal
     * - Shadow map cascade selection
     * - Portal and occlusion culling systems
     *
     * Planes are stored in the form `ax + by + cz + d = 0` where the normal `(a, b, c)` points
     * **inward** - a point is inside the frustum if it is on the positive side of all six planes.
     */
    class frustum
    {
    public:
        enum plane_index : int
        {
            k_left = 0,
            k_right = 1,
            k_bottom = 2,
            k_top = 3,
            k_near = 4,
            k_far = 5,
        };

    public:
        constexpr frustum() noexcept = default;

        constexpr frustum(
            const plane& left,
            const plane& right,
            const plane& bottom,
            const plane& top,
            const plane& near,
            const plane& far
        ) noexcept
            : planes{left, right, bottom, top, near, far}
        {
        }

        /**
         * @brief Constructs a frustum from a combined view-projection matrix.
         *
         * Extracts and normalizes the six clipping planes from the given matrix.
         * Planes are oriented so that normals point inward.
         *
         * @param view_proj The combined view-projection matrix.
         * @return A frustum with normalized inward-facing planes.
         */
        [[nodiscard]] static frustum from_matrix(const math::mat4& view_proj) noexcept;

        /**
         * @brief Checks if a point is inside the frustum.
         *
         * @param point The point to test.
         * @return true if the point is inside or on the boundary of all six planes.
         */
        [[nodiscard]] bool contains_point(const math::vec3& point) const noexcept
        {
            for (const auto& p : planes) {
                if (math::dot(p.normal, point) + p.distance < 0.0f) {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief Checks if a sphere intersects or is inside the frustum.
         *
         * @param s The sphere to test.
         * @return true if the sphere is at least partially inside the frustum.
         */
        [[nodiscard]] bool contains_sphere(const sphere& s) const noexcept
        {
            for (const auto& p : planes) {
                if (math::dot(p.normal, s.center) + p.distance < -s.radius) {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief Checks if an AABB intersects or is inside the frustum.
         *
         * Uses the "positive vertex" method - for each plane, finds the AABB corner
         * most aligned with the plane normal and tests it against the plane.
         *
         * @param box The axis-aligned bounding box to test.
         * @return true if the box is at least partially inside the frustum.
         */
        [[nodiscard]] bool contains_aabb(const aabb3& box) const noexcept;

    public:
        plane planes[6];
    };

} // namespace tavros::geometry

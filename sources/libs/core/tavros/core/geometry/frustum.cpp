#include <tavros/core/geometry/frustum.hpp>

namespace tavros::geometry
{
    frustum frustum::from_matrix(const math::mat4& m) noexcept
    {
        // Gribb/Hartmann method - extract planes directly from matrix rows
        // Each plane is: row3 +/- rowN, then normalize
        // Matrix is assumed column-major (as in GLM)

        auto r0 = math::vec3(m[0][0], m[1][0], m[2][0]);
        auto r1 = math::vec3(m[0][1], m[1][1], m[2][1]);
        auto r2 = math::vec3(m[0][2], m[1][2], m[2][2]);
        auto r3 = math::vec3(m[0][3], m[1][3], m[2][3]);

        float w0 = m[0][3];
        float w1 = m[1][3];
        float w2 = m[2][3];
        float w3 = m[3][3];

        auto make_plane = [](const math::vec3& normal, float d) -> plane {
            float len = math::length(normal);
            return plane(normal / len, d / len);
        };

        plane left = make_plane(r3 + r0, w3 + w0);
        plane right = make_plane(r3 - r0, w3 - w0);
        plane bottom = make_plane(r3 + r1, w3 + w1);
        plane top = make_plane(r3 - r1, w3 - w1);
        plane near = make_plane(r3 + r2, w3 + w2);
        plane far = make_plane(r3 - r2, w3 - w2);

        return frustum(left, right, bottom, top, near, far);
    }

    bool frustum::contains_aabb(const aabb3& box) const noexcept
    {
        for (const auto& p : planes) {
            // Find the "positive vertex" - the AABB corner most aligned with the plane normal.
            // If even this corner is outside, the entire box is outside.
            math::vec3 positive_vertex(
                p.normal.x >= 0.0f ? box.max.x : box.min.x,
                p.normal.y >= 0.0f ? box.max.y : box.min.y,
                p.normal.z >= 0.0f ? box.max.z : box.min.z
            );

            if (math::dot(p.normal, positive_vertex) + p.distance < 0.0f) {
                return false;
            }
        }
        return true;
    }

} // namespace tavros::geometry

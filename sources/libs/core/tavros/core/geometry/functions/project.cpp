#include <tavros/core/geometry/functions/project.hpp>

#include <tavros/core/geometry/functions/distance.hpp>
#include <tavros/core/geometry/plane.hpp>
#include <tavros/core/geometry/sphere.hpp>

namespace tavros::geometry
{

    math::vec3 project_point(const math::vec3& point, const plane& plane) noexcept
    {
        return point - plane.normal * distance(plane, point);
    }

    math::vec3 project_point(const math::vec3& point, const sphere& sphere) noexcept
    {
        const auto dir = point - sphere.center;
        const auto sq_len = math::squared_length(dir);

        if (sq_len == 0.0f) {
            // Direction along Z axis, positive
            return sphere.center + math::vec3(0.0f, 0.0f, sphere.radius);
        }

        if (sq_len < sphere.radius * sphere.radius) {
            return sphere.center + normalize(dir) * sphere.radius;
        }

        return sphere.center + dir * (sphere.radius / math::sqrt(sq_len));
    }

} // namespace tavros::geometry


#include <tavros/core/geometry/functions/project.hpp>

#include <tavros/core/geometry/functions/distance.hpp>
#include <tavros/core/geometry/plane.hpp>

namespace tavros::geometry
{

    math::vec3 project_point(const plane& plane, const math::vec3& point) noexcept
    {
        return point - plane.normal * distance(plane, point);
    }

} // namespace tavros::geometry


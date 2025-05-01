#include <tavros/core/geometry/functions/distance.hpp>

#include <tavros/core/geometry/plane.hpp>
#include <tavros/core/geometry/sphere.hpp>

namespace tavros::geometry
{

    float distance(const plane& plane, const math::vec3& point) noexcept
    {
        return math::dot(plane.normal, point) + plane.d;
    }

    float distance(const sphere& sphere, const math::vec3& point) noexcept
    {
        return math::length(point - sphere.center) - sphere.radius;
    }

} // namespace tavros::geometry

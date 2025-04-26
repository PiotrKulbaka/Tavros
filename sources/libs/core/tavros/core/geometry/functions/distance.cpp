#include <tavros/core/geometry/functions/distance.hpp>

#include <tavros/core/geometry/plane.hpp>

namespace tavros::geometry
{

    float distance(const plane& plane, const math::vec3& point) noexcept
    {
        return math::dot(plane.normal, point) + plane.d;
    }

} // namespace tavros::geometry


#include <tavros/core/geometry/plane.hpp>

using namespace tavros::geometry;
using namespace tavros::math;

plane::plane(const vec3& a, const vec3& b, const vec3& c) noexcept
{
    const auto ab = b - a;
    const auto ac = c - a;

    normal = ab.cross(ac).normalized();
    d = -normal.dot(a);
}

float plane::distance(const vec3& point) const noexcept
{
    return normal.dot(point) + d;
}

vec3 plane::project_point(const vec3& point) const noexcept
{
    return point - normal * distance(point);
}

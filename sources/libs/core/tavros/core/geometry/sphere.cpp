#include <tavros/core/geometry/sphere.hpp>

using namespace tavros::geometry;
using namespace tavros::math;

float sphere::distance(const vec3& point) const noexcept
{
    return (point - center).length() - radius;
}

float sphere::squared_distance(const vec3& point) const noexcept
{
    return (point - center).squared_length() - radius * radius;
}

vec3 sphere::project_point(const vec3& point) const noexcept
{
    const auto dir = point - center;
    const auto len = dir.length();

    if (len < radius) {
        return center + dir.normalized() * radius;
    }

    if (len == 0.0f) {
        // Direction along Z axis, positive
        return center + vec3(0.0f, 0.0f, radius);
    }

    return center + dir * (radius / len);
}

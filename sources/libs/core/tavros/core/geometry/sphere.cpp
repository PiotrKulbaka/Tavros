#include <tavros/core/geometry/sphere.hpp>

using namespace tavros::geometry;
using namespace tavros::math;

float sphere::distance(const vec3& point) const noexcept
{
    return length(point - center) - radius;
}

float sphere::squared_distance(const vec3& point) const noexcept
{
    return squared_length(point - center) - radius * radius;
}

vec3 sphere::project_point(const vec3& point) const noexcept
{
    const auto dir = point - center;
    const auto len = length(dir);

    if (len < radius) {
        return center + normalize(dir) * radius;
    }

    if (len == 0.0f) {
        // Direction along Z axis, positive
        return center + vec3(0.0f, 0.0f, radius);
    }

    return center + dir * (radius / len);
}

#include <tavros/core/math/aabb3.hpp>

#include <limits>

using namespace tavros::core::math;

aabb3::aabb3() noexcept
    : min(std::numeric_limits<float>::max())
    , max(std::numeric_limits<float>::lowest())
{
}

aabb3::aabb3(const vec3& min_point, const vec3& max_point) noexcept
    : min(min_point)
    , max(max_point)
{
}

vec3 aabb3::center() const noexcept
{
    return (min + max) * 0.5f;
}

vec3 aabb3::size() const noexcept
{
    return max - min;
}

float aabb3::volume() const noexcept
{
    const auto s = size();
    return s.x * s.y * s.z;
}

bool aabb3::contains_point(const vec3& point) const noexcept
{
    return (point.x >= min.x && point.x <= max.x)
        && (point.y >= min.y && point.y <= max.y)
        && (point.z >= min.z && point.z <= max.z);
}

bool aabb3::intersects(const aabb3& other) const noexcept
{
    return (min.x <= other.max.x && max.x >= other.min.x)
        && (min.y <= other.max.y && max.y >= other.min.y)
        && (min.z <= other.max.z && max.z >= other.min.z);
}

void aabb3::expand(const vec3& point) noexcept
{
    min = min.min(point);
    max = max.max(point);
}

aabb3 aabb3::merged(const aabb3& other) const noexcept
{
    return aabb3(min.min(other.min), max.max(other.max));
}

void aabb3::merge(const aabb3& other) noexcept
{
    min = min.min(other.min);
    max = max.max(other.max);
}

void aabb3::reset() noexcept
{
    min = vec3(std::numeric_limits<float>::max());
    max = vec3(std::numeric_limits<float>::lowest());
}

float aabb3::distance(const vec3& point) const noexcept
{
    float dx = std::max({min.x - point.x, 0.0f, point.x - max.x});
    float dy = std::max({min.y - point.y, 0.0f, point.y - max.y});
    float dz = std::max({min.z - point.z, 0.0f, point.z - max.z});
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

aabb3 aabb3::transformed(const mat4& transform) const noexcept
{
    // 8 corners of the box
    vec4 corners[8] = {
        vec4(min.x, min.y, min.z, 1.0f),
        vec4(max.x, min.y, min.z, 1.0f),
        vec4(min.x, max.y, min.z, 1.0f),
        vec4(max.x, max.y, min.z, 1.0f),
        vec4(min.x, min.y, max.z, 1.0f),
        vec4(max.x, min.y, max.z, 1.0f),
        vec4(min.x, max.y, max.z, 1.0f),
        vec4(max.x, max.y, max.z, 1.0f)
    };

    aabb3 result;
    for (const auto& corner : corners) {
        auto t = transform * corner; // Just ignore the w component
        result.expand(vec3(t.x, t.y, t.z));
    }

    return result;
}

#include <tavros/core/geometry/aabb2.hpp>

#include <limits>

#include <tavros/core/math/functions/max.hpp>
#include <tavros/core/math/functions/min.hpp>

using namespace tavros::geometry;
using namespace tavros;

aabb2::aabb2() noexcept
    : min(std::numeric_limits<float>::max())
    , max(std::numeric_limits<float>::lowest())
{
}

aabb2::aabb2(const math::vec2& min_point, const math::vec2& max_point) noexcept
    : min(min_point)
    , max(max_point)
{
}

aabb2::aabb2(float left, float top, float right, float bottom) noexcept
    : min(left, top)
    , max(right, bottom)
{
}

math::vec2 aabb2::center() const noexcept
{
    return (min + max) * 0.5f;
}

math::vec2 aabb2::size() const noexcept
{
    return max - min;
}

float aabb2::area() const noexcept
{
    const math::vec2 s = size();
    return s.x * s.y;
}

bool aabb2::contains_point(const math::vec2& point) const noexcept
{
    return (point.x >= min.x && point.y >= min.y)
        && (point.x <= max.x && point.y <= max.y);
}

void aabb2::expand(const math::vec2& point) noexcept
{
    min = tavros::math::min(min, point);
    max = tavros::math::max(max, point);
}

aabb2 aabb2::merged(const aabb2& other) const noexcept
{
    return aabb2(tavros::math::min(min, other.min), tavros::math::max(max, other.max));
}

void aabb2::merge(const aabb2& other) noexcept
{
    min = tavros::math::min(min, other.min);
    max = tavros::math::max(max, other.max);
}

void aabb2::reset() noexcept
{
    min = math::vec2(std::numeric_limits<float>::max());
    max = math::vec2(std::numeric_limits<float>::lowest());
}

float aabb2::distance(const math::vec2& point) const noexcept
{
    const float dx = tavros::math::max(min.x - point.x, 0.0f, point.x - max.x);
    const float dy = tavros::math::max(min.y - point.y, 0.0f, point.y - max.y);
    return std::sqrt(dx * dx + dy * dy);
}

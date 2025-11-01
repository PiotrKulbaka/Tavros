#include <tavros/core/geometry/aabb3.hpp>

#include <limits>

#include <tavros/core/math/functions/max.hpp>
#include <tavros/core/math/functions/min.hpp>

namespace tavros::geometry
{

    aabb3::aabb3() noexcept
        : min(std::numeric_limits<float>::max())
        , max(std::numeric_limits<float>::lowest())
    {
    }

    aabb3::aabb3(const math::vec3& min_point, const math::vec3& max_point) noexcept
        : min(min_point)
        , max(max_point)
    {
    }

    math::vec3 aabb3::center() const noexcept
    {
        return (min + max) * 0.5f;
    }

    math::vec3 aabb3::size() const noexcept
    {
        return max - min;
    }

    float aabb3::volume() const noexcept
    {
        const auto s = size();
        return s.x * s.y * s.z;
    }

    bool aabb3::contains_point(const math::vec3& point) const noexcept
    {
        return (point.x >= min.x && point.x <= max.x)
            && (point.y >= min.y && point.y <= max.y)
            && (point.z >= min.z && point.z <= max.z);
    }

    void aabb3::expand(const math::vec3& point) noexcept
    {
        min = tavros::math::min(this->min, point);
        max = tavros::math::max(this->max, point);
    }

    aabb3 aabb3::merged(const aabb3& other) const noexcept
    {
        return aabb3(tavros::math::min(min, other.min), tavros::math::max(max, other.max));
    }

    void aabb3::merge(const aabb3& other) noexcept
    {
        min = tavros::math::min(this->min, other.min);
        max = tavros::math::max(this->max, other.max);
    }

    void aabb3::reset() noexcept
    {
        min = math::vec3(std::numeric_limits<float>::max());
        max = math::vec3(std::numeric_limits<float>::lowest());
    }

    float aabb3::distance(const math::vec3& point) const noexcept
    {
        float dx = tavros::math::max(min.x - point.x, 0.0f, point.x - max.x);
        float dy = tavros::math::max(min.y - point.y, 0.0f, point.y - max.y);
        float dz = tavros::math::max(min.z - point.z, 0.0f, point.z - max.z);
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    aabb3 aabb3::transformed(const math::mat4& transform) const noexcept
    {
        // 8 corners of the box
        math::vec4 corners[8] = {
            math::vec4(min.x, min.y, min.z, 1.0f),
            math::vec4(max.x, min.y, min.z, 1.0f),
            math::vec4(min.x, max.y, min.z, 1.0f),
            math::vec4(max.x, max.y, min.z, 1.0f),
            math::vec4(min.x, min.y, max.z, 1.0f),
            math::vec4(max.x, min.y, max.z, 1.0f),
            math::vec4(min.x, max.y, max.z, 1.0f),
            math::vec4(max.x, max.y, max.z, 1.0f)
        };

        aabb3 result;
        for (const auto& corner : corners) {
            auto t = transform * corner; // Just ignore the w component
            result.expand(math::vec3(t.x, t.y, t.z));
        }

        return result;
    }

} // namespace tavros::geometry

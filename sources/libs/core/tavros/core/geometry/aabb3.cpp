#include <tavros/core/geometry/aabb3.hpp>

#include <tavros/core/math/functions/max.hpp>
#include <tavros/core/math/functions/min.hpp>

namespace tavros::geometry
{

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

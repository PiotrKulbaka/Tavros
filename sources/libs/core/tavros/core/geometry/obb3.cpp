#include <tavros/core/geometry/obb3.hpp>

#include <tavros/core/math/functions/max.hpp>
#include <tavros/core/math/functions/min.hpp>
#include <tavros/core/math/functions/dot.hpp>

namespace tavros::geometry
{

    bool obb3::contains_point(const math::vec3& point) const noexcept
    {
        math::vec3 d = point - center;

        // Checking for location within axis forward
        auto dist_forward = math::dot(d, forward);
        if (math::abs(dist_forward) > half_extents.x) {
            return false;
        }

        // Checking for location within axis right
        auto dist_right = math::dot(d, right);
        if (math::abs(dist_right) > half_extents.y) {
            return false;
        }

        // Checking for location within axis up
        auto dist_up = math::dot(d, up);
        if (math::abs(dist_up) > half_extents.z) {
            return false;
        }

        return true;
    }

} // namespace tavros::geometry

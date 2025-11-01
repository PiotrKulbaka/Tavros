#include <tavros/core/geometry/obb2.hpp>

#include <tavros/core/math/functions/dot.hpp>

#include <cmath>

namespace tavros::geometry
{

    bool obb2::contains_point(const math::vec2& point) const noexcept
    {
        auto d = point - center;

        // Checking for location within axis right
        auto dist_right = math::dot(d, right);
        if (std::abs(dist_right) > half_extents.x) {
            return false;
        }

        // Checking for location within axis up
        auto dist_up = math::dot(d, up);
        if (std::abs(dist_up) > half_extents.y) {
            return false;
        }

        return true;
    }

} // namespace tavros::geometry

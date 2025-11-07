#include <tavros/core/geometry/obb2.hpp>

#include <tavros/core/math/functions/dot.hpp>

#include <cmath>

namespace tavros::geometry
{

    bool obb2::contains_point(const math::vec2& point) const noexcept
    {
        auto det = right.x * up.y - right.y * up.x;
        if (std::abs(det) < math::k_epsilon6) {
            return false; // The basis is degenerate
        }

        auto d = point - center;

        auto x = (d.x * up.y - d.y * up.x) / det;
        if (std::abs(x) > half_extents.x) {
            return false;
        }

        float y = (right.x * d.y - right.y * d.x) / det;
        if (std::abs(y) > half_extents.y) {
            return false;
        }

        return true;
    }

} // namespace tavros::geometry

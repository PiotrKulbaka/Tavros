#include <tavros/core/geometry/obb2.hpp>

#include <tavros/core/math/functions/dot.hpp>

#include <cmath>

namespace tavros::geometry
{

    bool obb2::contains_point(const math::vec2& point) const noexcept
    {
        const math::vec3 d = point - center;

        const float sq_r = right.sq_length();
        const float sq_u = up.sq_length();

        if (math::abs(math::dot(d, right)) > sq_r) {
            return false;
        }

        if (math::abs(math::dot(d, up)) > sq_u) {
            return false;
        }

        return true;
    }

} // namespace tavros::geometry

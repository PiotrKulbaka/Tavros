#include <tavros/core/geometry/obb3.hpp>

#include <tavros/core/math/functions/max.hpp>
#include <tavros/core/math/functions/min.hpp>
#include <tavros/core/math/functions/dot.hpp>

namespace tavros::geometry
{

    bool obb3::contains_point(const math::vec3& point) const noexcept
    {
        const math::vec3 d = point - center;

        const float sq_f = forward.sq_length();
        const float sq_r = right.sq_length();
        const float sq_u = up.sq_length();

        if (math::abs(math::dot(d, forward)) > sq_f) {
            return false;
        }

        if (math::abs(math::dot(d, right)) > sq_r) {
            return false;
        }

        if (math::abs(math::dot(d, up)) > sq_u) {
            return false;
        }

        return true;
    }

} // namespace tavros::geometry

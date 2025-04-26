#include <tavros/core/math/functions/angle.hpp>

#include <tavros/core/math/functions/length.hpp>
#include <tavros/core/math/functions/dot.hpp>
#include <tavros/core/math/vec3.hpp>

namespace tavros::math
{

    float angle_between(const vec3& a, const vec3& b) noexcept
    {
        auto a_len = length(a);
        TAV_ASSERT(!almost_zero(a_len, k_epsilon6));
        if (almost_zero(a_len, k_epsilon6)) {
            return 0.0f;
        }

        auto b_len = length(b);
        TAV_ASSERT(!almost_zero(b_len, k_epsilon6));
        if (almost_zero(b_len, k_epsilon6)) {
            return 0.0f;
        }

        auto cos_angle = dot(a, b) / (a_len * b_len);
        cos_angle = std::fmax(-1.0f, std::fmin(1.0f, cos_angle));

        return acos(cos_angle);
    }

} // namespace tavros::math


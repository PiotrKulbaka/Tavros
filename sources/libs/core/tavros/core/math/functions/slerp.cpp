#include <tavros/core/math/functions/slerp.hpp>

#include <tavros/core/math/functions/dot.hpp>
#include <tavros/core/math/functions/normalize.hpp>

namespace tavros::math
{

    quat slerp(const quat& a, const quat& b, float coef) noexcept
    {
        auto cos_theta = dot(a, b);
        auto to = b;
        if (cos_theta < 0.0f) {
            to = -to;
            cos_theta = -cos_theta;
        }
        if (cos_theta > 0.9995f) {
            return normalize(a * (1.0f - coef) + to * coef);
        }
        auto theta = acos(cos_theta);
        auto sin_theta = sqrt(1.0f - cos_theta * cos_theta);
        auto sa = sin((1.0f - coef) * theta) / sin_theta;
        auto sb = sin(coef * theta) / sin_theta;
        return a * sa + to * sb;
    }

} // namespace tavros::math

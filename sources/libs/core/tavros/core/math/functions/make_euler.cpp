#include <tavros/core/math/functions/make_euler.hpp>

#include <tavros/core/math/functions/basic_math.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/euler3.hpp>

namespace tavros::math
{

    euler3 make_euler3(const quat& q) noexcept
    {
        auto sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
        auto cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
        auto pitch = atan2(sinr_cosp, cosr_cosp);

        auto sinp = 2.0f * (q.w * q.y - q.z * q.x);
        auto yaw = abs(sinp) >= 1.0f ? copysignf(k_half_pi<float>, sinp) : asin(sinp);

        auto siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
        auto cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
        auto roll = atan2(siny_cosp, cosy_cosp);

        return euler3(pitch, yaw, roll);
    }

} // namespace tavros::math

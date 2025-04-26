#include <tavros/core/math/functions/normalize.hpp>

#include <tavros/core/math/functions/length.hpp>

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/euler3.hpp>

namespace tavros::math
{

    euler3 normalize(const euler3& a) noexcept
    {
        return euler3(wrap_angle(a.roll), wrap_angle(a.pitch), wrap_angle(a.yaw));
    }

    vec2 normalize(const vec2& v) noexcept
    {
        auto len = length(v);
        TAV_ASSERT(len > 0.0f);
        if (len == 0.0f) {
            return vec2(0.0f, 1.0f);
        }
        return v / len;
    }

    vec3 normalize(const vec3& v) noexcept
    {
        auto len = length(v);
        TAV_ASSERT(len > 0.0f);
        if (len == 0.0f) {
            return vec3(0.0f, 0.0f, 1.0f);
        }
        return v / len;
    }

    vec4 normalize(const vec4& v) noexcept
    {
        auto len = length(v);
        TAV_ASSERT(len > 0.0f);
        if (len == 0.0f) {
            return vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }
        return v / len;
    }

    quat normalize(const quat& q) noexcept
    {
        auto len = length(q);
        TAV_ASSERT(len > 0.0f);
        if (len == 0.0f) {
            return quat(0.0f, 0.0f, 0.0f, 1.0f);
        }
        return q / len;
    }

} // namespace tavros::math

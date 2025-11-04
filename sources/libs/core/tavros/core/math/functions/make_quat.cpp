#include <tavros/core/math/functions/make_quat.hpp>

#include <tavros/core/math/euler3.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/quat.hpp>

#include <tavros/core/math/functions/normalize.hpp>
#include <tavros/core/math/functions/dot.hpp>
#include <tavros/core/math/functions/orthogonal.hpp>
#include <tavros/core/math/functions/cross.hpp>

namespace tavros::math
{

    quat make_quat(const vec3& axis, float angle_rad) noexcept
    {
        auto half = angle_rad * 0.5f;
        auto s = sin(half);
        auto n = normalize(axis);
        return quat(n.x * s, n.y * s, n.z * s, cos(half));
    }

    quat make_quat(const euler3& euler) noexcept
    {
        auto cx = cos(euler.roll * 0.5f);
        auto sx = sin(euler.roll * 0.5f);
        auto cy = cos(euler.pitch * 0.5f);
        auto sy = sin(euler.pitch * 0.5f);
        auto cz = cos(euler.yaw * 0.5f);
        auto sz = sin(euler.yaw * 0.5f);

        return quat(
            sx * cy * cz + cx * sy * sz,
            cx * sy * cz - sx * cy * sz,
            cx * cy * sz + sx * sy * cz,
            cx * cy * cz - sx * sy * sz
        );
    }

    quat make_quat(const vec3& from, const vec3& to) noexcept
    {
        auto dot_val = dot(from, to);
        if (dot_val >= 1.0f - k_epsilon6) {
            return quat::identity();
        }
        if (dot_val <= -1.0f + k_epsilon6) {
            auto axis = normalize(orthogonal(from));
            return quat::from_axis_angle(axis, k_pi<float>);
        }
        auto axis = cross(from, to);
        auto s = sqrt((1.0f + dot_val) * 2.0f);
        auto inv_s = 1.0f / s;
        return normalize(quat(axis.x * inv_s, axis.y * inv_s, axis.z * inv_s, s * 0.5f));
    }

    quat make_quat_forward_up(const vec3& forward, const vec3& up) noexcept
    {
        auto f = normalize(forward);
        auto r = cross(normalize(up), f);
        auto u = cross(f, r);

        quat  q;
        float trace = f.x + r.y + u.z;

        if (trace > 0.0f) {
            float s = sqrt(trace + 1.0f) * 2.0f;
            q.w = 0.25f * s;
            q.x = (r.z - u.y) / s;
            q.y = (u.x - f.z) / s;
            q.z = (f.y - r.x) / s;
        } else if ((f.x > r.y) && (f.x > u.z)) {
            float s = sqrt(1.0f + f.x - r.y - u.z) * 2.0f;
            q.w = (r.z - u.y) / s;
            q.x = 0.25f * s;
            q.y = (r.x + f.y) / s;
            q.z = (u.x + f.z) / s;
        } else if (r.y > u.z) {
            float s = sqrt(1.0f + r.y - f.x - u.z) * 2.0f;
            q.w = (u.x - f.z) / s;
            q.x = (r.x + f.y) / s;
            q.y = 0.25f * s;
            q.z = (u.y + r.z) / s;
        } else {
            float s = sqrt(1.0f + u.z - f.x - r.y) * 2.0f;
            q.w = (f.y - r.x) / s;
            q.x = (u.x + f.z) / s;
            q.y = (u.y + r.z) / s;
            q.z = 0.25f * s;
        }

        return normalize(q);
    }

} // namespace tavros::math

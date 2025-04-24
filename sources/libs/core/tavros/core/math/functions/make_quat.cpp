#include <tavros/core/math/functions/make_quat.hpp>

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
        return quat(axis.x * s, axis.y * s, axis.z * s, cos(half));
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
            return quat::from_axis_angle(axis, k_pi);
        }
        auto axis = cross(from, to);
        auto s = sqrt((1.0f + dot_val) * 2.0f);
        auto inv_s = 1.0f / s;
        return normalize(quat(axis.x * inv_s, axis.y * inv_s, axis.z * inv_s, s * 0.5f));
    }

    quat make_quat_froward_up(const vec3& forward, const vec3& up) noexcept
    {
        auto f = normalize(forward);
        auto r = normalize(cross(up, f));
        auto u = cross(f, r);
        auto trace = r.x + u.y + f.z;

        if (trace > 0.0f) {
            auto s = sqrt(trace + 1.0f) * 2.0f;
            return quat(
                (f.y - u.z) / s,
                (r.z - f.x) / s,
                (u.x - r.y) / s,
                0.25f * s
            );
        }

        if (r.x > u.y && r.x > f.z) {
            auto s = sqrt(1.0f + r.x - u.y - f.z) * 2.0f;
            return quat(
                0.25f * s,
                (r.y + u.x) / s,
                (r.z + f.x) / s,
                (f.y - u.z) / s
            );
        }

        if (u.y > f.z) {
            auto s = sqrt(1.0f + u.y - r.x - f.z) * 2.0f;
            return quat(
                (r.y + u.x) / s,
                0.25f * s,
                (u.z + f.y) / s,
                (r.z - f.x) / s
            );
        }

        auto s = sqrt(1.0f + f.z - r.x - u.y) * 2.0f;
        return quat(
            (r.z + f.x) / s,
            (u.z + f.y) / s,
            0.25f * s,
            (u.x - r.y) / s
        );
    }

} // namespace tavros::math

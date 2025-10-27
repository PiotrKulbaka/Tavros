#include <tavros/core/math/functions/make_mat.hpp>

#include <tavros/core/math/functions/cross.hpp>
#include <tavros/core/math/functions/normalize.hpp>
#include <tavros/core/math/functions/dot.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/mat3.hpp>
#include <tavros/core/math/mat4.hpp>

namespace tavros::math
{

    mat3 make_mat3(const quat& q) noexcept
    {
        auto xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        auto xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        auto wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

        return mat3(
            1.0f - 2.0f * (yy + zz),
            2.0f * (xy + wz),
            2.0f * (xz - wy),

            2.0f * (xy - wz),
            1.0f - 2.0f * (xx + zz),
            2.0f * (yz + wx),

            2.0f * (xz + wy),
            2.0f * (yz - wx),
            1.0f - 2.0f * (xx + yy)
        );
    }

    mat4 make_mat4(const quat& q) noexcept
    {
        auto xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        auto xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        auto wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

        return mat4(
            1.0f - 2.0f * (yy + zz),
            2.0f * (xy + wz),
            2.0f * (xz - wy),
            0.0f,

            2.0f * (xy - wz),
            1.0f - 2.0f * (xx + zz),
            2.0f * (yz + wx),
            0.0f,

            2.0f * (xz + wy),
            2.0f * (yz - wx),
            1.0f - 2.0f * (xx + yy),
            0.0f,

            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

    mat4 make_look_at_dir(const vec3& origin, const vec3& dir, const vec3& up) noexcept
    {
        const auto f = normalize(dir);
        const auto r = normalize(cross(f, up));
        const auto u = cross(r, f);

        return mat4(
            r.x, u.x, -f.x, 0.0f,
            r.y, u.y, -f.y, 0.0f,
            r.z, u.z, -f.z, 0.0f,
            -dot(r, origin), -dot(u, origin), dot(f, origin), 1.0f
        );
    }

    mat4 make_perspective(float fov_y, float aspect, float z_near, float z_far) noexcept
    {
        const float tan_half_fovy = tan(fov_y * 0.5f);
        const float sax = 1.0f / (aspect * tan_half_fovy);             // Scale axis X
        const float say = 1.0f / tan_half_fovy;                        // Scale axis Y
        const float saz = -(z_far + z_near) / (z_far - z_near);        // Scale axis Z
        const float dtt = -(2.0f * z_far * z_near) / (z_far - z_near); // Depth translation term

        return mat4(
            sax, 0.0f, 0.0f, 0.0f,
            0.0f, say, 0.0f, 0.0f,
            0.0f, 0.0f, saz, -1.0f,
            0.0f, 0.0f, dtt, 0.0f
        );
    }

    mat4 make_ortho(float left, float right, float bottom, float top, float z_near, float z_far) noexcept
    {
        const float inv_width = 1.0f / (right - left);
        const float inv_height = 1.0f / (top - bottom);
        const float inv_depth = 1.0f / (z_far - z_near);
        const float sax = 2.0f * inv_width;              // Scale axis X
        const float say = 2.0f * inv_height;             // Scale axis Y
        const float saz = -2.0f * inv_depth;             // Scale axis Z
        const float shx = -(right + left) * inv_width;   // Shift by X
        const float shy = -(top + bottom) * inv_height;  // Shift by Y
        const float shz = -(z_far + z_near) * inv_depth; // Shift by Z

        return mat4(
            sax, 0.0f, 0.0f, 0.0f,
            0.0f, say, 0.0f, 0.0f,
            0.0f, 0.0f, saz, 0.0f,
            shx, shy, shz, 1.0f
        );
    }

} // namespace tavros::math

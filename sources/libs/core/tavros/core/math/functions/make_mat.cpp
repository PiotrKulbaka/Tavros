#include <tavros/core/math/functions/make_mat.hpp>

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
            1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy),
            2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx),
            2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy)
        );
    }

    mat4 make_mat4(const quat& q) noexcept
    {
        auto xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        auto xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        auto wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
        return mat4(
            1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
            2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx), 0.0f,
            2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy), 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );
    }

} // namespace tavros::math

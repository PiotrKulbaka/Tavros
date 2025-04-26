#include <tavros/core/math/quat.hpp>

#include <tavros/core/math/functions/make_quat.hpp>
#include <tavros/core/math/functions/normalize.hpp>
#include <tavros/core/math/functions/rotate.hpp>

namespace tavros::math
{

    static_assert(sizeof(quat) == 16, "incorrect size");
    static_assert(alignof(quat) == 16, "incorrect alignment");

    inline constexpr quat quat::identity() noexcept
    {
        return quat(0.0f, 0.0f, 0.0f, 1.0f);
    }

    inline quat quat::from_axis_angle(const vec3& axis, float angle_rad) noexcept
    {
        return make_quat(axis, angle_rad);
    }

    inline quat quat::from_euler(const euler3& euler) noexcept
    {
        return make_quat(euler);
    }

    inline quat quat::from_to_rotation(const vec3& from, const vec3& to) noexcept
    {
        return make_quat(from, to);
    }

    inline quat quat::look_rotation(const vec3& forward, const vec3& up) noexcept
    {
        return make_quat_forward_up(forward, up);
    }

    inline constexpr quat::quat() noexcept
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
        , w(1.0f)
    {
    }

    inline constexpr quat::quat(float x, float y, float z, float w) noexcept
        : x(x)
        , y(y)
        , z(z)
        , w(w)
    {
    }

    inline constexpr float quat::operator[](size_t index) const noexcept
    {
        TAV_ASSERT(index < 4);
        return (&x)[index];
    }

    inline constexpr float& quat::operator[](size_t index) noexcept
    {
        TAV_ASSERT(index < 4);
        return (&x)[index];
    }

    inline constexpr quat& quat::operator+=(const quat& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    inline constexpr quat& quat::operator-=(const quat& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    inline constexpr quat& quat::operator*=(const quat& other) noexcept
    {
        *this = *this * other;
        return *this;
    }

    inline constexpr quat& quat::operator*=(float s) noexcept
    {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }

    inline constexpr quat& quat::operator/=(float s) noexcept
    {
        TAV_ASSERT(s != 0.0f);
        x /= s;
        y /= s;
        z /= s;
        w /= s;
        return *this;
    }

    inline vec3 quat::axis() const noexcept
    {
        auto q = normalize(*this);
        if (auto s = sqrt(1.0f - q.w * q.w); s > k_epsilon6) {
            return vec3(q.x / s, q.y / s, q.z / s);
        }
        return vec3(1.0f, 0.0f, 0.0f);
    }

    inline float quat::angle() const noexcept
    {
        auto q = normalize(*this);
        return 2.0f * acos(q.w);
    }

    inline constexpr const float* quat::data() const noexcept
    {
        return &x;
    }

    inline constexpr float* quat::data() noexcept
    {
        return &x;
    }

    inline constexpr quat operator-(const quat& q) noexcept
    {
        return quat(-q.x, -q.y, -q.z, -q.w);
    }

    inline constexpr quat operator+(const quat& a, const quat& b) noexcept
    {
        return quat(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
    }

    inline constexpr quat operator-(const quat& a, const quat& b) noexcept
    {
        return quat(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
    }

    inline constexpr quat operator*(const quat& a, const quat& b) noexcept
    {
        return quat(
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
        );
    }

    inline vec3 operator*(const quat& q, const vec3& p) noexcept
    {
        return rotate_point(q, p);
    }

    inline constexpr quat operator*(const quat& q, float s) noexcept
    {
        return quat(q.x * s, q.y * s, q.z * s, q.w * s);
    }

    inline constexpr quat operator*(float s, const quat& q) noexcept
    {
        return quat(q.x * s, q.y * s, q.z * s, q.w * s);
    }

    inline constexpr quat operator/(const quat& q, float s) noexcept
    {
        TAV_ASSERT(s != 0.0f);
        return quat(q.x / s, q.y / s, q.z / s, q.w / s);
    }

    inline constexpr quat operator/(float s, const quat& q) noexcept
    {
        TAV_ASSERT(q.x != 0.0f && q.y != 0.0f && q.z != 0.0f && q.w != 0.0f);
        return quat(s / q.x, s / q.y, s / q.z, s / q.w);
    }

} // namespace tavros::math

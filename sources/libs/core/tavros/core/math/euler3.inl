#include <tavros/core/math/euler3.hpp>

#include "functions/make_euler.hpp"

namespace tavros::math
{

    static_assert(sizeof(euler3) == 12, "incorrect size");
    static_assert(alignof(euler3) == 4, "incorrect alignment");

    //    euler3 euler3::from_quat(const quat& q) noexcept
    //    {
    //        return make_euler3(q);
    //    }

    inline constexpr euler3::euler3() noexcept
        : roll(0.0f)
        , pitch(0.0f)
        , yaw(0.0f)
    {
    }

    inline constexpr euler3::euler3(float roll, float pitch, float yaw) noexcept
        : roll(roll)
        , pitch(pitch)
        , yaw(yaw)
    {
    }

    inline constexpr euler3::euler3(const vec3& v) noexcept
        : roll(v.x)
        , pitch(v.y)
        , yaw(v.z)
    {
    }

    inline constexpr float euler3::operator[](size_t index) const noexcept
    {
        TAV_ASSERT(index < 3);
        return vec[index];
    }

    inline constexpr float& euler3::operator[](size_t index) noexcept
    {
        TAV_ASSERT(index < 3);
        return vec[index];
    }

    inline constexpr euler3& euler3::operator+=(const euler3& other) noexcept
    {
        roll += other.roll;
        pitch += other.pitch;
        yaw += other.yaw;
        return *this;
    }

    inline constexpr euler3& euler3::operator-=(const euler3& other) noexcept
    {
        roll -= other.roll;
        pitch -= other.pitch;
        yaw -= other.yaw;
        return *this;
    }

    inline constexpr euler3& euler3::operator*=(float scalar) noexcept
    {
        roll *= scalar;
        pitch *= scalar;
        yaw *= scalar;
        return *this;
    }

    inline constexpr const float* euler3::data() const noexcept
    {
        return vec.data();
    }

    inline constexpr float* euler3::data() noexcept
    {
        return vec.data();
    }

    inline constexpr euler3 operator-(const euler3& e) noexcept
    {
        return euler3(-e.roll, -e.pitch, -e.yaw);
    }

    inline constexpr euler3 operator+(const euler3& a, const euler3& b) noexcept
    {
        return euler3(a.roll + b.roll, a.pitch + b.pitch, a.yaw + b.yaw);
    }

    inline constexpr euler3 operator-(const euler3& a, const euler3& b) noexcept
    {
        return euler3(a.roll - b.roll, a.pitch - b.pitch, a.yaw - b.yaw);
    }

    inline constexpr euler3 operator*(const euler3& a, const euler3& b) noexcept
    {
        return euler3(a.roll * b.roll, a.pitch * b.pitch, a.yaw * b.yaw);
    }

    inline constexpr euler3 operator*(const euler3& e, float s) noexcept
    {
        return euler3(e.roll * s, e.pitch * s, e.yaw * s);
    }

    inline constexpr euler3 operator*(float s, const euler3& e) noexcept
    {
        return euler3(e.roll * s, e.pitch * s, e.yaw * s);
    }

    inline constexpr euler3 operator/(const euler3& a, const euler3& b) noexcept
    {
        TAV_ASSERT(b.roll != 0.0f && b.pitch != 0.0f && b.yaw != 0.0f);
        return euler3(a.roll / b.roll, a.pitch / b.pitch, a.yaw / b.yaw);
    }

    inline constexpr euler3 operator/(const euler3& e, float s) noexcept
    {
        TAV_ASSERT(s != 0.0f);
        return euler3(e.roll / s, e.pitch / s, e.yaw / s);
    }

    inline constexpr euler3 operator/(float s, const euler3& e) noexcept
    {
        TAV_ASSERT(e.roll != 0.0f && e.pitch != 0.0f && e.yaw != 0.0f);
        return euler3(s / e.roll, s / e.pitch, s / e.yaw);
    }

} // namespace tavros::math


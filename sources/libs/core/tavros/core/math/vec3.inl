#include <tavros/core/math/vec3.hpp>

namespace tavros::math
{

    static_assert(sizeof(vec3) == 12, "incorrect size");
    static_assert(alignof(vec3) == 4, "incorrect alignment");

    inline constexpr vec3::vec3() noexcept
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
    {
    }

    inline constexpr vec3::vec3(float v) noexcept
        : x(v)
        , y(v)
        , z(v)
    {
    }

    inline constexpr vec3::vec3(vec2 v, float z) noexcept
        : x(v.x)
        , y(v.y)
        , z(z)
    {
    }

    inline constexpr vec3::vec3(float x, float y, float z) noexcept
        : x(x)
        , y(y)
        , z(z)
    {
    }

    inline constexpr float vec3::operator[](size_t index) const noexcept
    {
        TAV_ASSERT(index < 3);
        return (&x)[index];
    }

    inline constexpr float& vec3::operator[](size_t index) noexcept
    {
        TAV_ASSERT(index < 3);
        return (&x)[index];
    }

    inline constexpr vec3& vec3::operator+=(const vec3& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    inline constexpr vec3& vec3::operator-=(const vec3& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    inline constexpr vec3& vec3::operator*=(float a) noexcept
    {
        x *= a;
        y *= a;
        z *= a;
        return *this;
    }

    inline constexpr vec3& vec3::operator/=(float a) noexcept
    {
        TAV_ASSERT(a != 0.0f);
        x /= a;
        y /= a;
        z /= a;
        return *this;
    }

    inline constexpr vec3& vec3::operator/=(const vec3& other) noexcept
    {
        TAV_ASSERT(other.x != 0.0f && other.y != 0.0f && other.z != 0.0f);
        x /= other.x;
        y /= other.y;
        z /= other.z;
        return *this;
    }

    inline constexpr void vec3::set(float x, float y, float z) noexcept
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    inline constexpr const float* vec3::data() const noexcept
    {
        return &x;
    }

    inline constexpr float* vec3::data() noexcept
    {
        return &x;
    }

    inline constexpr vec3 operator-(const vec3& v) noexcept
    {
        return vec3(-v.x, -v.y, -v.z);
    }

    inline constexpr vec3 operator+(const vec3& a, const vec3& b) noexcept
    {
        return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
    }

    inline constexpr vec3 operator-(const vec3& a, const vec3& b) noexcept
    {
        return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
    }

    inline constexpr vec3 operator*(const vec3& a, const vec3& b) noexcept
    {
        return vec3(a.x * b.x, a.y * b.y, a.z * b.z);
    }

    inline constexpr vec3 operator*(const vec3& v, float s) noexcept
    {
        return vec3(v.x * s, v.y * s, v.z * s);
    }

    inline constexpr vec3 operator*(float s, const vec3& v) noexcept
    {
        return vec3(v.x * s, v.y * s, v.z * s);
    }

    inline constexpr vec3 operator/(const vec3& a, const vec3& b) noexcept
    {
        TAV_ASSERT(b.x != 0.0f && b.y != 0.0f && b.z != 0.0f);
        return vec3(a.x / b.x, a.y / b.y, a.z / b.z);
    }

    inline constexpr vec3 operator/(const vec3& v, float s) noexcept
    {
        TAV_ASSERT(s != 0.0f);
        return vec3(v.x / s, v.y / s, v.z / s);
    }

    inline constexpr vec3 operator/(float s, const vec3& v) noexcept
    {
        TAV_ASSERT(v.x != 0.0f && v.y != 0.0f && v.z != 0.0f);
        return vec3(s / v.x, s / v.y, s / v.z);
    }

} // namespace tavros::math

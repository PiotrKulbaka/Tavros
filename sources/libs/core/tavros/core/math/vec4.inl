#include <tavros/core/math/vec4.hpp>

namespace tavros::math
{

    static_assert(sizeof(vec4) == 16, "incorrect size");
    static_assert(alignof(vec4) == 16, "incorrect alignment");

    inline constexpr vec4::vec4() noexcept
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
        , w(0.0f)
    {
    }

    inline constexpr vec4::vec4(float v) noexcept
        : x(v)
        , y(v)
        , z(v)
        , w(v)
    {
    }

    inline constexpr vec4::vec4(vec2 v, float z, float w) noexcept
        : x(v.x)
        , y(v.y)
        , z(z)
        , w(w)
    {
    }

    inline constexpr vec4::vec4(vec3 v, float w) noexcept
        : x(v.x)
        , y(v.y)
        , z(v.z)
        , w(w)
    {
    }

    inline constexpr vec4::vec4(float x, float y, float z, float w) noexcept
        : x(x)
        , y(y)
        , z(z)
        , w(w)
    {
    }

    inline constexpr float vec4::operator[](size_t index) const noexcept
    {
        TAV_ASSERT(index < 4);
        return (&x)[index];
    }

    inline constexpr float& vec4::operator[](size_t index) noexcept
    {
        TAV_ASSERT(index < 4);
        return (&x)[index];
    }

    inline constexpr vec4& vec4::operator+=(const vec4& other) noexcept
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    inline constexpr vec4& vec4::operator-=(const vec4& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    inline constexpr vec4& vec4::operator*=(float a) noexcept
    {
        x *= a;
        y *= a;
        z *= a;
        w *= a;
        return *this;
    }

    inline constexpr vec4& vec4::operator/=(float a) noexcept
    {
        TAV_ASSERT(a != 0.0f);
        x /= a;
        y /= a;
        z /= a;
        w /= a;
        return *this;
    }

    inline constexpr vec4& vec4::operator/=(const vec4& other) noexcept
    {
        TAV_ASSERT(other.x != 0.0f && other.y != 0.0f && other.z != 0.0f && other.w != 0.0f);
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;
        return *this;
    }

    inline constexpr void vec4::set(float x, float y, float z, float w) noexcept
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    }

    inline constexpr const float* vec4::data() const noexcept
    {
        return &x;
    }

    inline constexpr float* vec4::data() noexcept
    {
        return &x;
    }

    inline constexpr vec4 operator-(const vec4& v) noexcept
    {
        return vec4(-v.x, -v.y, -v.z, -v.w);
    }

    inline constexpr vec4 operator+(const vec4& a, const vec4& b) noexcept
    {
        return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
    }

    inline constexpr vec4 operator-(const vec4& a, const vec4& b) noexcept
    {
        return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
    }

    inline constexpr vec4 operator*(const vec4& a, const vec4& b) noexcept
    {
        return vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
    }

    inline constexpr vec4 operator*(const vec4& v, float s) noexcept
    {
        return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
    }

    inline constexpr vec4 operator*(float s, const vec4& v) noexcept
    {
        return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
    }

    inline constexpr vec4 operator/(const vec4& a, const vec4& b) noexcept
    {
        TAV_ASSERT(b.x != 0.0f && b.y != 0.0f && b.z != 0.0f && b.w != 0.0f);
        return vec4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
    }

    inline constexpr vec4 operator/(const vec4& v, float s) noexcept
    {
        TAV_ASSERT(s != 0.0f);
        return vec4(v.x / s, v.y / s, v.z / s, v.w / s);
    }

    inline constexpr vec4 operator/(float s, const vec4& v) noexcept
    {
        TAV_ASSERT(v.x != 0.0f && v.y != 0.0f && v.z != 0.0f && v.w != 0.0f);
        return vec4(s / v.x, s / v.y, s / v.z, s / v.w);
    }

} // namespace tavros::math

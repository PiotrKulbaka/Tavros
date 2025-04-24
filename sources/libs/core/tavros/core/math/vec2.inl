#include <tavros/core/math/vec2.hpp>

namespace tavros::math
{

    static_assert(sizeof(vec2) == 8, "incorrect size");
    static_assert(alignof(vec2) == 4, "incorrect alignment");

    inline constexpr vec2::vec2() noexcept
        : x(0.0f)
        , y(0.0f)
    {
    }

    inline constexpr vec2::vec2(float v) noexcept
        : x(v)
        , y(v)
    {
    }

    inline constexpr vec2::vec2(float x, float y) noexcept
        : x(x)
        , y(y)
    {
    }

    inline constexpr float vec2::operator[](size_t index) const noexcept
    {
        TAV_ASSERT(index < 2);
        return (&x)[index];
    }

    inline constexpr float& vec2::operator[](size_t index) noexcept
    {
        TAV_ASSERT(index < 2);
        return (&x)[index];
    }

    inline constexpr vec2& vec2::operator+=(const vec2& other) noexcept
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    inline constexpr vec2& vec2::operator-=(const vec2& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    inline constexpr vec2& vec2::operator*=(float s) noexcept
    {
        x *= s;
        y *= s;
        return *this;
    }

    inline constexpr vec2& vec2::operator/=(float s) noexcept
    {
        TAV_ASSERT(s != 0.0f);
        x /= s;
        y /= s;
        return *this;
    }

    inline constexpr vec2& vec2::operator/=(const vec2& other) noexcept
    {
        TAV_ASSERT(other.x != 0.0f && other.y != 0.0f);
        x /= other.x;
        y /= other.y;
        return *this;
    }

    inline constexpr const float* vec2::data() const noexcept
    {
        return &x;
    }

    inline constexpr float* vec2::data() noexcept
    {
        return &x;
    }

    inline constexpr vec2 operator-(const vec2& v) noexcept
    {
        return vec2(-v.x, -v.y);
    }

    inline constexpr vec2 operator+(const vec2& a, const vec2& b) noexcept
    {
        return vec2(a.x + b.x, a.y + b.y);
    }

    inline constexpr vec2 operator-(const vec2& a, const vec2& b) noexcept
    {
        return vec2(a.x - b.x, a.y - b.y);
    }

    inline constexpr vec2 operator*(const vec2& a, const vec2& b) noexcept
    {
        return vec2(a.x * b.x, a.y * b.y);
    }

    inline constexpr vec2 operator*(const vec2& v, float s) noexcept
    {
        return vec2(v.x * s, v.y * s);
    }

    inline constexpr vec2 operator*(float s, const vec2& v) noexcept
    {
        return vec2(v.x * s, v.y * s);
    }

    inline constexpr vec2 operator/(const vec2& a, const vec2& b) noexcept
    {
        TAV_ASSERT(b.x != 0.0f && b.y != 0.0f);
        return vec2(a.x / b.x, a.y / b.y);
    }

    inline constexpr vec2 operator/(const vec2& v, float s) noexcept
    {
        TAV_ASSERT(s != 0.0f);
        return vec2(v.x / s, v.y / s);
    }

    inline constexpr vec2 operator/(float s, const vec2& v) noexcept
    {
        TAV_ASSERT(v.x != 0.0f && v.y != 0.0f);
        return vec2(s / v.x, s / v.y);
    }

} // namespace tavros::math

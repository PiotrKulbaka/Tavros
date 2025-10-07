#include <tavros/core/math/ivec2.hpp>

#include <tavros/core/debug/assert.hpp>

namespace tavros::math
{

    static_assert(sizeof(ivec2) == 8, "incorrect size");
    static_assert(alignof(ivec2) == 4, "incorrect alignment");

    inline constexpr ivec2::ivec2() noexcept
        : x(0)
        , y(0)
    {
    }

    inline constexpr ivec2::ivec2(int32 x, int32 y) noexcept
        : x(x)
        , y(y)
    {
    }

    inline constexpr ivec2& ivec2::operator+=(const ivec2& other) noexcept
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    inline constexpr ivec2& ivec2::operator*=(const ivec2& other) noexcept
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    inline constexpr ivec2& ivec2::operator*=(int32 s) noexcept
    {
        x *= s;
        y *= s;
        return *this;
    }

    inline constexpr ivec2& ivec2::operator-=(const ivec2& other) noexcept
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    inline constexpr ivec2& ivec2::operator/=(const ivec2& other) noexcept
    {
        TAV_ASSERT(other.x != 0 && other.y != 0);
        x /= other.x;
        y /= other.y;
        return *this;
    }

    inline constexpr ivec2& ivec2::operator/=(int32 s) noexcept
    {
        TAV_ASSERT(s != 0);
        x /= s;
        y /= s;
        return *this;
    }

    inline constexpr ivec2 operator-(const ivec2& v) noexcept
    {
        return ivec2(-v.x, -v.y);
    }

    inline constexpr ivec2 operator+(const ivec2& a, const ivec2& b) noexcept
    {
        return ivec2(a.x + b.x, a.y + b.y);
    }

    inline constexpr ivec2 operator-(const ivec2& a, const ivec2& b) noexcept
    {
        return ivec2(a.x - b.x, a.y - b.y);
    }

    inline constexpr ivec2 operator*(const ivec2& a, const ivec2& b) noexcept
    {
        return ivec2(a.x * b.x, a.y * b.y);
    }

    inline constexpr ivec2 operator*(const ivec2& v, int32 s) noexcept
    {
        return ivec2(v.x * s, v.y * s);
    }

    inline constexpr ivec2 operator*(int32 s, const ivec2& v) noexcept
    {
        return ivec2(v.x * s, v.y * s);
    }

    inline constexpr ivec2 operator/(const ivec2& a, const ivec2& b) noexcept
    {
        TAV_ASSERT(b.x != 0 && b.y != 0);
        return ivec2(a.x / b.x, a.y / b.y);
    }

    inline constexpr ivec2 operator/(const ivec2& v, int32 s) noexcept
    {
        TAV_ASSERT(s != 0);
        return ivec2(v.x / s, v.y / s);
    }

    inline constexpr ivec2 operator/(int32 s, const ivec2& v) noexcept
    {
        TAV_ASSERT(v.x != 0 && v.y != 0);
        return ivec2(s / v.x, s / v.y);
    }

} // namespace tavros::math

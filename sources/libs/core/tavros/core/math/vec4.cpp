#include <tavros/core/math/vec4.hpp>

#include <tavros/core/debug/assert.hpp>

#include <cmath>

using namespace tavros::core::math;

constexpr float vec4::operator[](size_t index) const noexcept
{
    TAV_ASSERT(index < 4);
    return (&x)[index];
}

constexpr float& vec4::operator[](size_t index) noexcept
{
    TAV_ASSERT(index < 4);
    return (&x)[index];
}

constexpr vec4& vec4::operator+=(const vec4& other) noexcept
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

constexpr vec4& vec4::operator-=(const vec4& other) noexcept
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

constexpr vec4& vec4::operator*=(float a) noexcept
{
    x *= a;
    y *= a;
    z *= a;
    w *= a;
    return *this;
}

constexpr vec4& vec4::operator/=(float a) noexcept
{
    TAV_ASSERT(a != 0.0f);
    x /= a;
    y /= a;
    z /= a;
    w /= a;
    return *this;
}

constexpr vec4& vec4::operator/=(const vec4& other) noexcept
{
    TAV_ASSERT(other.x != 0.0f && other.y != 0.0f && other.z != 0.0f && other.w != 0.0f);
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return *this;
}

constexpr vec4 vec4::operator-() const noexcept
{
    return vec4(-x, -y, -z, -w);
}

constexpr vec4 vec4::operator+(const vec4& other) const noexcept
{
    return vec4(x + other.x, y + other.y, z + other.z, w + other.w);
}

constexpr vec4 vec4::operator-(const vec4& other) const noexcept
{
    return vec4(x - other.x, y - other.y, z - other.z, w - other.w);
}

constexpr vec4 vec4::operator*(const vec4& other) const noexcept
{
    return vec4(x * other.x, y * other.y, z * other.z, w * other.w);
}

constexpr vec4 vec4::operator*(float a) const noexcept
{
    return vec4(x * a, y * a, z * a, w * a);
}

constexpr vec4 vec4::operator/(float a) const noexcept
{
    TAV_ASSERT(a != 0.0f);
    return vec4(x / a, y / a, z / a, w / a);
}

constexpr bool vec4::operator==(const vec4& other) const noexcept
{
    return x == other.x && y == other.y && z == other.z && w == other.w;
}

constexpr bool vec4::operator!=(const vec4& other) const noexcept
{
    return !(*this == other);
}

constexpr float vec4::dot(const vec4& other) const noexcept
{
    return x * other.x + y * other.y + z * other.z + w * other.w;
}

constexpr vec4 vec4::lerp(const vec4& target, float coef) const noexcept
{
    return *this + (target - *this) * coef;
}

float vec4::length() const noexcept
{
    return std::sqrt(dot(*this));
}

vec4 vec4::normalized() const noexcept
{
    if (float len = length(); len != 0.0f) {
        return *this / len;
    }
    return vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

constexpr const float* vec4::ptr() const noexcept
{
    return &x;
}

constexpr float* vec4::ptr() noexcept
{
    return &x;
}

tavros::core::string vec4::to_string(int precision) const
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f, %.*f]", precision, x, precision, y, precision, z, precision, w);
    return string(buffer);
}

#include <tavros/core/math/vec3.hpp>
#include <cmath>

using namespace tavros::core::math;

constexpr vec3::vec3(float v) noexcept
    : x(v)
    , y(v)
    , z(v)
{
}

constexpr vec3::vec3(float x, float y, float z) noexcept
    : x(x)
    , y(y)
    , z(z)
{
}

constexpr float vec3::operator[](size_t index) const noexcept
{
    return ptr()[index];
}

constexpr float& vec3::operator[](size_t index) noexcept
{
    return ptr()[index];
}

constexpr vec3& vec3::operator+=(const vec3& other) noexcept
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

constexpr vec3& vec3::operator-=(const vec3& other) noexcept
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

constexpr vec3& vec3::operator*=(float a) noexcept
{
    x *= a;
    y *= a;
    z *= a;
    return *this;
}

constexpr vec3& vec3::operator/=(float a) noexcept
{
    x /= a;
    y /= a;
    z /= a;
    return *this;
}

constexpr vec3& vec3::operator/=(const vec3& other) noexcept
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}

constexpr vec3 vec3::operator-() const noexcept
{
    return vec3(-x, -y, -z);
}

constexpr vec3 vec3::operator+(const vec3& other) const noexcept
{
    return vec3(x + other.x, y + other.y, z + other.z);
}

constexpr vec3 vec3::operator-(const vec3& other) const noexcept
{
    return vec3(x - other.x, y - other.y, z - other.z);
}

constexpr vec3 vec3::operator*(const vec3& other) const noexcept
{
    return vec3(x * other.x, y * other.y, z * other.z);
}

constexpr vec3 vec3::operator*(float a) const noexcept
{
    return vec3(x * a, y * a, z * a);
}

constexpr vec3 vec3::operator/(float a) const noexcept
{
    return vec3(x / a, y / a, z / a);
}

constexpr bool vec3::operator==(const vec3& other) const noexcept
{
    return x == other.x && y == other.y && z == other.z;
}

constexpr bool vec3::operator!=(const vec3& other) const noexcept
{
    return !(*this == other);
}

constexpr float vec3::dot(const vec3& other) const noexcept
{
    return x * other.x + y * other.y + z * other.z;
}

constexpr vec3 vec3::lerp(const vec3& target, float coef) const noexcept
{
    return *this + (target - *this) * coef;
}

float vec3::length() const noexcept
{
    return std::sqrt(dot(*this));
}

vec3 vec3::normalized() const noexcept
{
    if (float len = length(); len != 0.0f) {
        return *this / len;
    }
    return vec3(0.0f, 0.0f, 1.0f);
}

constexpr const float* vec3::ptr() const noexcept
{
    return &x;
}

constexpr float* vec3::ptr() noexcept
{
    return &x;
}

tavros::core::string vec3::to_string(int precision) const
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f]", precision, x, precision, y, precision, z);
    return string(buffer);
}

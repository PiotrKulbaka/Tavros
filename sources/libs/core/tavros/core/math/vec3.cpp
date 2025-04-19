#include <tavros/core/math/vec3.hpp>

#include <tavros/core/debug/assert.hpp>

#include <cmath>

using namespace tavros::core::math;

float vec3::operator[](size_t index) const noexcept
{
    TAV_ASSERT(index < 3);
    return ptr()[index];
}

float& vec3::operator[](size_t index) noexcept
{
    TAV_ASSERT(index < 3);
    return ptr()[index];
}

vec3& vec3::operator+=(const vec3& other) noexcept
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

vec3& vec3::operator-=(const vec3& other) noexcept
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

vec3& vec3::operator*=(float a) noexcept
{
    x *= a;
    y *= a;
    z *= a;
    return *this;
}

vec3& vec3::operator/=(float a) noexcept
{
    TAV_ASSERT(a != 0.0f);
    x /= a;
    y /= a;
    z /= a;
    return *this;
}

vec3& vec3::operator/=(const vec3& other) noexcept
{
    TAV_ASSERT(other.x != 0.0f && other.y != 0.0f && other.z != 0.0f);
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}

vec3 vec3::operator-() const noexcept
{
    return vec3(-x, -y, -z);
}

vec3 vec3::operator+(const vec3& other) const noexcept
{
    return vec3(x + other.x, y + other.y, z + other.z);
}

vec3 vec3::operator-(const vec3& other) const noexcept
{
    return vec3(x - other.x, y - other.y, z - other.z);
}

vec3 vec3::operator*(const vec3& other) const noexcept
{
    return vec3(x * other.x, y * other.y, z * other.z);
}

vec3 vec3::operator*(float a) const noexcept
{
    return vec3(x * a, y * a, z * a);
}

vec3 vec3::operator/(float a) const noexcept
{
    TAV_ASSERT(a != 0.0f);
    return vec3(x / a, y / a, z / a);
}

bool vec3::operator==(const vec3& other) const noexcept
{
    return x == other.x && y == other.y && z == other.z;
}

bool vec3::operator!=(const vec3& other) const noexcept
{
    return !(*this == other);
}

bool vec3::almost_equal(const vec3& other, float epsilon) const noexcept
{
    // clang-format off
    return std::abs(x - other.x) <= epsilon
        && std::abs(y - other.y) <= epsilon
        && std::abs(z - other.z) <= epsilon;
    // clang-format on
}

vec3 vec3::cross(const vec3& other) const noexcept
{
    return vec3(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

float vec3::dot(const vec3& other) const noexcept
{
    return x * other.x + y * other.y + z * other.z;
}

vec3 vec3::lerp(const vec3& target, float coef) const noexcept
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

const float* vec3::ptr() const noexcept
{
    return &x;
}

float* vec3::ptr() noexcept
{
    return &x;
}

tavros::core::string vec3::to_string(int precision) const
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f]", precision, x, precision, y, precision, z);
    return string(buffer);
}

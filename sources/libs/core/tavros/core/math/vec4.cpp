#include <tavros/core/math/vec4.hpp>

#include <tavros/core/debug/assert.hpp>

#include <cmath>

using namespace tavros::core::math;

float vec4::operator[](size_t index) const noexcept
{
    TAV_ASSERT(index < 4);
    return (&x)[index];
}

float& vec4::operator[](size_t index) noexcept
{
    TAV_ASSERT(index < 4);
    return (&x)[index];
}

vec4& vec4::operator+=(const vec4& other) noexcept
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

vec4& vec4::operator-=(const vec4& other) noexcept
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

vec4& vec4::operator*=(float a) noexcept
{
    x *= a;
    y *= a;
    z *= a;
    w *= a;
    return *this;
}

vec4& vec4::operator/=(float a) noexcept
{
    TAV_ASSERT(a != 0.0f);
    x /= a;
    y /= a;
    z /= a;
    w /= a;
    return *this;
}

vec4& vec4::operator/=(const vec4& other) noexcept
{
    TAV_ASSERT(other.x != 0.0f && other.y != 0.0f && other.z != 0.0f && other.w != 0.0f);
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return *this;
}

vec4 vec4::operator-() const noexcept
{
    return vec4(-x, -y, -z, -w);
}

vec4 vec4::operator+(const vec4& other) const noexcept
{
    return vec4(x + other.x, y + other.y, z + other.z, w + other.w);
}

vec4 vec4::operator-(const vec4& other) const noexcept
{
    return vec4(x - other.x, y - other.y, z - other.z, w - other.w);
}

vec4 vec4::operator*(const vec4& other) const noexcept
{
    return vec4(x * other.x, y * other.y, z * other.z, w * other.w);
}

vec4 vec4::operator*(float a) const noexcept
{
    return vec4(x * a, y * a, z * a, w * a);
}

vec4 vec4::operator/(float a) const noexcept
{
    TAV_ASSERT(a != 0.0f);
    return vec4(x / a, y / a, z / a, w / a);
}

bool vec4::operator==(const vec4& other) const noexcept
{
    return x == other.x && y == other.y && z == other.z && w == other.w;
}

bool vec4::operator!=(const vec4& other) const noexcept
{
    return !(*this == other);
}

bool vec4::almost_equal(const vec4& other, float epsilon) const noexcept
{
    // clang-format off
    return std::abs(x - other.x) <= epsilon
        && std::abs(y - other.y) <= epsilon
        && std::abs(z - other.z) <= epsilon
        && std::abs(w - other.w) <= epsilon;
    // clang-format on
}

float vec4::dot(const vec4& other) const noexcept
{
    return x * other.x + y * other.y + z * other.z + w * other.w;
}

vec4 vec4::lerp(const vec4& target, float coef) const noexcept
{
    return *this + (target - *this) * coef;
}

float vec4::length() const noexcept
{
    return std::sqrt(dot(*this));
}

vec4 vec4::normalized() const noexcept
{
    if (float len = length(); std::abs(len) > k_vec_normalize_epsilon) {
        return *this / len;
    }
    return vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

const float* vec4::data() const noexcept
{
    return &x;
}

float* vec4::data() noexcept
{
    return &x;
}

tavros::core::string vec4::to_string(int precision) const
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f, %.*f]", precision, x, precision, y, precision, z, precision, w);
    return string(buffer);
}

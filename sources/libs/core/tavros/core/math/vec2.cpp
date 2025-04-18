#include <tavros/core/math/vec2.hpp>

#include <tavros/core/debug/assert.hpp>

#include <cmath>

using namespace tavros::core::math;

float vec2::operator[](size_t index) const noexcept
{
    TAV_ASSERT(index < 2);
    return index == 0 ? x : y;
}

float& vec2::operator[](size_t index) noexcept
{
    TAV_ASSERT(index < 2);
    return index == 0 ? x : y;
}

vec2& vec2::operator+=(const vec2& other) noexcept
{
    x += other.x;
    y += other.y;
    return *this;
}

vec2& vec2::operator-=(const vec2& other) noexcept
{
    x -= other.x;
    y -= other.y;
    return *this;
}

vec2& vec2::operator*=(float a) noexcept
{
    x *= a;
    y *= a;
    return *this;
}

vec2& vec2::operator/=(float a) noexcept
{
    TAV_ASSERT(a != 0.0f);
    x /= a;
    y /= a;
    return *this;
}

vec2& vec2::operator/=(const vec2& other) noexcept
{
    TAV_ASSERT(other.x != 0.0f && other.y != 0.0f);
    x /= other.x;
    y /= other.y;
    return *this;
}

vec2 vec2::operator-() const noexcept
{
    return vec2(-x, -y);
}

vec2 vec2::operator+(const vec2& other) const noexcept
{
    return vec2(x + other.x, y + other.y);
}

vec2 vec2::operator-(const vec2& other) const noexcept
{
    return vec2(x - other.x, y - other.y);
}

vec2 vec2::operator*(const vec2& other) const noexcept
{
    return vec2(x * other.x, y * other.y);
}

vec2 vec2::operator*(float a) const noexcept
{
    return vec2(x * a, y * a);
}

vec2 vec2::operator/(float a) const noexcept
{
    TAV_ASSERT(a != 0.0f);
    return vec2(x / a, y / a);
}

bool vec2::operator==(const vec2& other) const noexcept
{
    return x == other.x && y == other.y;
}

bool vec2::operator!=(const vec2& other) const noexcept
{
    return !(*this == other);
}

bool vec2::almost_equal(const vec2& other, float epsilon) const noexcept
{
    return std::abs(x - other.x) <= epsilon && std::abs(y - other.y) <= epsilon;
}

float vec2::cross(const vec2& other) const noexcept
{
    return x * other.y - y * other.x;
}

float vec2::dot(const vec2& other) const noexcept
{
    return x * other.x + y * other.y;
}

vec2 vec2::lerp(const vec2& target, float coef) const noexcept
{
    return *this + (target - *this) * coef;
}

float vec2::length() const noexcept
{
    return std::sqrt(x * x + y * y);
}

vec2 vec2::normalized() const noexcept
{
    if (float len = length(); std::abs(len) > k_vec_normalize_epsilon) {
        return *this / len;
    }
    return vec2(0.0f, 1.0f);
}

const float* vec2::data() const noexcept
{
    return &x;
}

float* vec2::data() noexcept
{
    return &x;
}

tavros::core::string vec2::to_string(int precision) const
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[%.*f, %.*f]", precision, x, precision, y);
    return string(buffer);
}

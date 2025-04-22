#include <tavros/core/math/vec3.hpp>

#include <tavros/core/debug/assert.hpp>

using namespace tavros::math;

float vec3::operator[](size_t index) const noexcept
{
    TAV_ASSERT(index < 3);
    return data()[index];
}

float& vec3::operator[](size_t index) noexcept
{
    TAV_ASSERT(index < 3);
    return data()[index];
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

bool vec3::almost_equal(const vec3& other, float epsilon) const noexcept
{
    // clang-format off
    return ::almost_equal(x, other.x, epsilon)
        && ::almost_equal(y, other.y, epsilon)
        && ::almost_equal(z, other.z, epsilon);
    // clang-format on
}

float vec3::angle(const vec3& other) const noexcept
{
    auto len = length();
    TAV_ASSERT(!almost_zero(len, k_epsilon6));
    if (almost_zero(len, k_epsilon6)) {
        return 0.0f;
    }

    auto other_len = other.length();
    TAV_ASSERT(!almost_zero(other_len, k_epsilon6));
    if (almost_zero(other_len, k_epsilon6)) {
        return 0.0f;
    }

    auto cos_angle = dot(other) / (len * other_len);
    cos_angle = std::fmax(-1.0f, std::fmin(1.0f, cos_angle));

    return std::acos(cos_angle);
}

vec3 vec3::cross(const vec3& other) const noexcept
{
    return vec3(
        z * other.y - y * other.z,
        x * other.z - z * other.x,
        y * other.x - x * other.y
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

float vec3::squared_length() const noexcept
{
    return dot(*this);
}

vec3 vec3::normalized() const noexcept
{
    auto len = length();
    TAV_ASSERT(!almost_zero(len, k_epsilon6));
    if (almost_zero(len, k_epsilon6)) {
        return vec3(0.0f, 0.0f, 1.0f);
    }
    return *this / len;
}

vec3 vec3::orthogonal() const noexcept
{
    if (std::abs(x) < std::abs(y)) {
        if (std::abs(x) < std::abs(z)) {
            // X minimal
            return vec3(0.0f, -z, y);
        } else {
            // Z minimal
            return vec3(-y, x, 0.0f);
        }
    } else {
        if (std::abs(y) < std::abs(z)) {
            // Y minimal
            return vec3(-z, 0.0f, x);
        } else {
            // Z minimal
            return vec3(-y, x, 0.0f);
        }
    }
}

vec3 vec3::min(const vec3& other) const noexcept
{
    return vec3(
        std::min(x, other.x),
        std::min(y, other.y),
        std::min(z, other.z)
    );
}

vec3 vec3::max(const vec3& other) const noexcept
{
    return vec3(
        std::max(x, other.x),
        std::max(y, other.y),
        std::max(z, other.z)
    );
}

const float* vec3::data() const noexcept
{
    return &x;
}

float* vec3::data() noexcept
{
    return &x;
}

tavros::core::string vec3::to_string(int precision) const
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f]", precision, x, precision, y, precision, z);
    return tavros::core::string(buffer);
}

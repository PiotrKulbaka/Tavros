#include <tavros/core/math/euler3.hpp>

#include <tavros/core/debug/assert.hpp>

namespace tavros::core::math
{

    float euler3::operator[](size_t index) const noexcept
    {
        TAV_ASSERT(index < 3);
        return vec[index];
    }

    float& euler3::operator[](size_t index) noexcept
    {
        TAV_ASSERT(index < 3);
        return vec[index];
    }

    euler3& euler3::operator+=(const euler3& other) noexcept
    {
        vec += other.vec;
        return *this;
    }

    euler3& euler3::operator-=(const euler3& other) noexcept
    {
        vec -= other.vec;
        return *this;
    }

    euler3& euler3::operator*=(float scalar) noexcept
    {
        vec *= scalar;
        return *this;
    }

    euler3& euler3::operator/=(float scalar) noexcept
    {
        vec /= scalar;
        return *this;
    }

    euler3 euler3::operator-() const noexcept
    {
        return euler3(-roll, -pitch, -yaw);
    }

    euler3 euler3::operator+(const euler3& other) const noexcept
    {
        return euler3(vec + other.vec);
    }

    euler3 euler3::operator-(const euler3& other) const noexcept
    {
        return euler3(vec - other.vec);
    }

    euler3 euler3::operator*(float scalar) const noexcept
    {
        return euler3(vec * scalar);
    }

    euler3 euler3::operator/(float scalar) const noexcept
    {
        return euler3(vec / scalar);
    }

    bool euler3::almost_equal(const euler3& other, float epsilon) const noexcept
    {
        return vec.almost_equal(other.vec, epsilon);
    }

    euler3 euler3::normalized() const noexcept
    {
        return {
            wrap_angle(roll),
            wrap_angle(pitch),
            wrap_angle(yaw)
        };
    }

    const float* euler3::data() const noexcept
    {
        return vec.data();
    }

    float* euler3::data() noexcept
    {
        return vec.data();
    }

    tavros::core::string euler3::to_string(int precision) const
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f]", precision, roll, precision, pitch, precision, yaw);
        return string(buffer);
    }

} // namespace tavros::core::math

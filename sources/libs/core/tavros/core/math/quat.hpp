#pragma once

#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/mat3.hpp>
#include <tavros/core/math/mat4.hpp>

namespace tavros::core::math
{

    class quat
    {
    public:
        static quat  identity() noexcept;
        static quat  from_axis_angle(const vec3& axis, float angle_rad) noexcept;
        static quat  from_euler_xyz(float pitch, float yaw, float roll) noexcept;
        static quat  from_to_rotation(const vec3& from, const vec3& to) noexcept;
        static float dot(const quat& a, const quat& b) noexcept;
        static quat  slerp(const quat& a, const quat& b, float t) noexcept;
        static quat  lerp(const quat& a, const quat& b, float t) noexcept;

    public:
        quat() noexcept = default;
        quat(float x, float y, float z, float w) noexcept;
        quat(const vec3& axis, float angle_rad) noexcept;

        bool operator==(const quat& other) const noexcept;
        bool operator!=(const quat& other) const noexcept;

        quat operator-() const noexcept;
        quat operator+(const quat& other) const noexcept;
        quat operator-(const quat& other) const noexcept;
        quat operator*(const quat& other) const noexcept;
        quat operator*(float scalar) const noexcept;

        quat& operator+=(const quat& other) noexcept;
        quat& operator-=(const quat& other) noexcept;
        quat& operator*=(const quat& other) noexcept;
        quat& operator*=(float scalar) noexcept;

        float length() const noexcept;
        quat  normalized() const noexcept;
        void  normalize() noexcept;
        quat  conjugated() const noexcept;
        quat  inversed() const noexcept;

        vec3 rotate(const vec3& v) const noexcept;

        mat3 to_mat3() const noexcept;
        mat4 to_mat4() const noexcept;
        vec3 to_euler_xyz() const noexcept;
        void to_axis_angle(vec3& out_axis, float& out_angle_rad) const noexcept;

        const float* data() const noexcept;
        float*       data() noexcept;

        tavros::core::string to_string(int precision = 3) const;

    private:
        float x;
        float y;
        float z;
        float w;
    };

} // namespace tavros::core::math
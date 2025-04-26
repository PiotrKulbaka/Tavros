#pragma once

#include <tavros/core/math/vec4.hpp>

namespace tavros::math
{
    class euler3;
    class vec4;

    /**
     * @brief Quaternion for representing 3D rotations.
     *
     * Quaternions avoid gimbal lock and provide smooth interpolation (slerp).
     * This implementation uses left-handed coordinate system (X forward, Y right, Z up).
     * All matrices returned by this class are in column-major order.
     */
    class quat
    {
    public:
        /// @brief Returns identity quaternion (no rotation)
        static constexpr quat identity() noexcept;

        /**
         * @brief Constructs a quaternion from axis-angle rotation
         * @param axis Normalized axis
         * @param angle_rad Rotation angle in radians
         */
        static quat from_axis_angle(const vec3& axis, float angle_rad) noexcept;

        /**
         * @brief Constructs a quaternion from euler angles (XYZ order)
         * Assumes left-handed coordinate system (X forward, Y right, Z up)
         */
        static quat from_euler(const euler3& euler) noexcept;

        /**
         * @brief Constructs rotation quaternion to rotate from one vector to another.
         * @param from Source direction (normalized).
         * @param to Target direction (normalized).
         */
        static quat from_to_rotation(const vec3& from, const vec3& to) noexcept;

        /**
         * @brief Constructs a quaternion looking in the forward direction with a given up vector.
         * Assumes left-handed coordinate system (X forward, Y right, Z up).
         * @param forward Direction to look at (must be normalized).
         * @param up World up direction (must be normalized and not colinear with forward).
         */
        static quat look_rotation(const vec3& forward, const vec3& up) noexcept;

    public:
        /// @brief Default constructor. Produces identity quaternion (no rotation).
        constexpr quat() noexcept;

        /**
         * @brief Constructs a quaternion from components.
         * @param x X component (imaginary part)
         * @param y Y component (imaginary part)
         * @param z Z component (imaginary part)
         * @param w W component (real part)
         */
        constexpr quat(float x, float y, float z, float w) noexcept;

        /// @brief Accesses a component by index [0..3]. Asserts in debug if index is out of bounds
        constexpr float operator[](size_t index) const noexcept;

        /// @brief Accesses a component by index [0..3]. Asserts in debug if index is out of bounds
        constexpr float& operator[](size_t index) noexcept;

        /// @brief Deleted comparison. Use `almost_equal` instead
        bool operator==(const quat& other) const = delete;

        /// @brief Deleted comparison. Use `almost_equal` instead
        bool operator!=(const quat& other) const = delete;

        /// @brief Adds another quat to this one
        constexpr quat& operator+=(const quat& other) noexcept;

        /// @brief Subtracts another quat from this one
        constexpr quat& operator-=(const quat& other) noexcept;

        /// @brief Multiplies this quaternion by other
        constexpr quat& operator*=(const quat& other) noexcept;

        /// @brief Multiplies this quaternion by a scalar
        constexpr quat& operator*=(float scalar) noexcept;

        /// @brief Divides this vector by a scalar. Asserts in debug if `scalar` is zero
        constexpr quat& operator/=(float scalar) noexcept;

        /// @brief Returns normalized rotation axis
        vec3 axis() const noexcept;

        /// @brief Returns rotation angle in radians
        float angle() const noexcept;

        /// @brief Returns a pointer to the raw float array [x, y, z, w]
        constexpr const float* data() const noexcept;

        /// @brief Returns a pointer to the raw float array [x, y, z, w]
        constexpr float* data() noexcept;

    public:
        union
        {
            vec4 vec;
            struct
            {
                float x, y, z, w;
            };
        };
    };


    constexpr quat operator-(const quat& q) noexcept;
    constexpr quat operator+(const quat& a, const quat& b) noexcept;
    constexpr quat operator-(const quat& a, const quat& b) noexcept;
    constexpr quat operator*(const quat& a, const quat& b) noexcept;
    vec3           operator*(const quat& q, const vec3& p) noexcept;
    constexpr quat operator*(const quat& q, float s) noexcept;
    constexpr quat operator*(float s, const quat& q) noexcept;
    constexpr quat operator/(const quat& q, float s) noexcept;
    constexpr quat operator/(float s, const quat& q) noexcept;

} // namespace tavros::math

#include <tavros/core/math/quat.inl>

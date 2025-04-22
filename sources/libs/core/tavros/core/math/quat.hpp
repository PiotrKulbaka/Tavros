#pragma once

#include <tavros/core/math/euler3.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/mat3.hpp>
#include <tavros/core/math/mat4.hpp>

namespace tavros::math
{

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
        /**
         * @brief Default constructor. Produces identity quaternion (no rotation).
         */
        constexpr quat() noexcept;

        /**
         * @brief Constructs a quaternion from components.
         * @param x X component (imaginary part)
         * @param y Y component (imaginary part)
         * @param z Z component (imaginary part)
         * @param w W component (real part)
         */
        constexpr quat(float x, float y, float z, float w) noexcept;

        /**
         * @brief Constructs quaternion from euler angles (XYZ order).
         * @param euler Euler angles in radians (XYZ rotation order).
         */
        quat(const euler3& euler) noexcept;

        /**
         * @brief Constructs quaternion from axis-angle rotation.
         * @param axis Normalized axis of rotation.
         * @param angle_rad Angle of rotation in radians.
         */
        quat(const vec3& axis, float angle_rad) noexcept;

        /**
         * @brief Deleted comparison. Use `almost_equal` instead
         */
        bool operator==(const quat& other) const = delete;

        /**
         * @brief Deleted comparison. Use `almost_equal` instead
         */
        bool operator!=(const quat& other) const = delete;

        /**
         * @brief Compares two sets of quat with a given tolerance.
         *
         * Returns true if the absolute difference between corresponding components
         * of the two quat sets is less than or equal to the specified epsilon.
         *
         * This is useful for floating-point comparisons where exact equality is
         * not reliable.
         *
         * @param other The other quat instance to compare with.
         * @param epsilon The allowed difference per component. Default is k_epsilon6.
         * @return true if all components are approximately equal.
         */
        bool almost_equal(const quat& other, float epsilon = k_epsilon6) const noexcept;

        /**
         * @brief Adds another quat to this one
         */
        quat& operator+=(const quat& other) noexcept;

        /**
         * @brief Subtracts another quat from this one
         */
        quat& operator-=(const quat& other) noexcept;

        /**
         * @brief Multiplies this quaternion by other
         */
        quat& operator*=(const quat& other) noexcept;

        /**
         * @brief Multiplies this quaternion by a scalar
         */
        quat& operator*=(float scalar) noexcept;

        /**
         * @brief Negates all components (does not change rotation).
         * Used internally in slerp if shortest path is desired.
         */
        quat operator-() const noexcept;

        /**
         * @brief Adds another quat to this one
         */
        quat operator+(const quat& other) const noexcept;

        /**
         * @brief Subtracts another quat from this one
         */
        quat operator-(const quat& other) const noexcept;

        /**
         * @brief Multiplies this quaternion by other
         */
        quat operator*(const quat& other) const noexcept;

        /**
         * @brief Scales all components by a scalar value.
         */
        quat operator*(float scalar) const noexcept;

        /**
         * @brief Dot product between this and another quaternion.
         * @return Scalar dot product result.
         */
        float dot(const quat& other) const noexcept;

        /**
         * @brief Checks if quaternion is normalized (length == 1).
         */
        bool is_normalized() const noexcept;

        /**
         * @brief Returns the length of the quaternion
         */
        float length() const noexcept;

        /**
         * @brief Returns the normalized quaternion.
         * If the quaternion has zero length, identity quaternion is returned.
         * @note Asserts in debug builds if the quaternion has zero length.
         */
        quat normalized() const noexcept;

        /**
         * @brief Returns the conjugate of the quaternion.
         * Conjugate = inverse if the quaternion is normalized.
         */
        quat conjugated() const noexcept;

        /**
         * @brief Returns the inverse rotation.
         * Requires normalization for correct behavior.
         */
        quat inversed() const noexcept;

        /**
         * @brief Rotates a point using this quaternion.
         * Equivalent to applying the rotation represented by this quaternion to the point.
         * Assumes left-handed coordinate system.
         */
        vec3 rotate_point(const vec3& point) const noexcept;

        /**
         * @brief Spherical linear interpolation between this and target quaternion
         * @note Interpolates along the shortest path unless the dot product is negative
         * @param target Target rotation
         * @param coef Interpolation factor [0..1], but can be outside this range, not clamped
         * @return Interpolated rotation
         */
        quat slerp(const quat& target, float coef) const noexcept;

        /**
         * @brief Converts quaternion to 3x3 rotation matrix.
         * @note Matrix is column-major (OpenGL-style).
         *       Rotation assumes left-handed coordinate system.
         */
        mat3 to_mat3() const noexcept;

        /**
         * @brief Converts quaternion to 4x4 rotation matrix.
         * @note Matrix is column-major (OpenGL-style). Rotation is in upper-left 3x3.
         *       Assumes left-handed coordinate system (X forward, Y right, Z up).
         */
        mat4 to_mat4() const noexcept;

        /**
         * @brief Converts quaternion to euler angles in radians (XYZ rotation order).
         * Assumes left-handed coordinate system (X forward, Y right, Z up).
         */
        euler3 to_euler() const noexcept;

        /**
         * @brief Converts to axis-angle representation.
         * @param out_axis Output normalized rotation axis.
         * @return Rotation angle in radians.
         */
        float to_axis_angle(vec3& out_axis) const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [x, y, z, w]
         */
        const float* data() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [x, y, z, w]
         */
        float* data() noexcept;

        /**
         * @brief Returns a string representation "[x, y, z, w]" with specified precision
         */
        core::string to_string(int precision = 3) const;

    public:
        /**
         * @brief Returns identity quaternion (no rotation).
         */
        static constexpr quat identity() noexcept;

        /**
         * @brief Constructs a quaternion from axis-angle rotation.
         * @param axis Normalized axis.
         * @param angle_rad Rotation angle in radians.
         */
        static quat from_axis_angle(const vec3& axis, float angle_rad) noexcept;

        /**
         * @brief Constructs a quaternion from euler angles (XYZ order).
         * Assumes left-handed coordinate system (X forward, Y right, Z up).
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
        union
        {
            vec4 vec;
            struct
            {
                float x, y, z, w;
            };
        };
    };

    static_assert(sizeof(quat) == 16, "incorrect size");
    static_assert(alignof(quat) == 16, "incorrect alignment");

    inline constexpr quat::quat() noexcept
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
        , w(1.0f)
    {
    }

    inline constexpr quat::quat(float x, float y, float z, float w) noexcept
        : x(x)
        , y(y)
        , z(z)
        , w(w)
    {
    }

} // namespace tavros::math

#pragma once

#include <tavros/core/math/vec3.hpp>

namespace tavros::core::math
{

    /**
     * @brief Represents a set of Euler angles in radians.
     *
     * This class stores and manipulates a set of 3D Euler angles — roll, pitch, and yaw —
     * representing rotations around the X, Y, and Z axes, respectively. All angles are stored
     * in radians.
     *
     * Euler angles are commonly used to represent orientation in 3D space, but they are subject
     * to limitations such as gimbal lock and discontinuities. When interpolation or composition
     * of rotations is needed, consider converting to quaternions.
     *
     * This class provides arithmetic operations, normalization, and approximate equality checks,
     * but deliberately disables strict equality comparison due to floating-point precision issues.
     *
     * The layout of the class allows accessing angles both by name (roll, pitch, yaw) and
     * as a `vec3`.
     */
    class euler3
    {
    public:
        /**
         * @brief Default constructor
         */
        constexpr euler3() noexcept;

        /**
         * @brief Constructs a euler3 from individual components
         */
        constexpr euler3(float roll, float pitch, float yaw) noexcept;

        /**
         * @brief Constructs a euler3 from a vec3
         */
        explicit constexpr euler3(const vec3& v) noexcept;

        /**
         * @brief Accesses a component by index [0..2]
         * @note Asserts in debug if index is out of bounds
         */
        float operator[](size_t index) const noexcept;

        /**
         * @brief Accesses a component by index [0..2]
         * @note Asserts in debug if index is out of bounds
         */
        float& operator[](size_t index) noexcept;

        /**
         * @brief Adds another angles to this one
         */
        euler3& operator+=(const euler3& other) noexcept;

        /**
         * @brief Subtracts another angles from this one
         */
        euler3& operator-=(const euler3& other) noexcept;

        /**
         * @brief Multiplies this angles by a scalar
         */
        euler3& operator*=(float scalar) noexcept;

        /**
         * @brief Divides this angles by a scalar
         * @note Asserts in debug if `a` is zero
         */
        euler3& operator/=(float scalar) noexcept;

        /**
         * @brief Returns the negative of this angles
         */
        euler3 operator-() const noexcept;

        /**
         * @brief Adds another angles to this one
         */
        euler3 operator+(const euler3& other) const noexcept;

        /**
         * @brief Subtracts another angles from this one
         */
        euler3 operator-(const euler3& other) const noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        euler3 operator*(float scalar) const noexcept;

        /**
         * @brief Divides this angles by a scalar
         * @note Asserts in debug if `a` is zero
         */
        euler3 operator/(float scalar) const noexcept;

        /**
         * @brief Deleted comparison. Use `almost_equal` instead
         */
        bool operator==(const euler3& other) const = delete;

        /**
         * @brief Deleted comparison. Use `almost_equal` instead
         */
        bool operator!=(const euler3& other) const = delete;

        /**
         * @brief Compares two sets of Euler angles with a given tolerance.
         *
         * Returns true if the absolute difference between corresponding components
         * of the two angle sets is less than or equal to the specified epsilon.
         *
         * This is useful for floating-point comparisons where exact equality is
         * not reliable.
         *
         * @param other The other euler3 instance to compare with.
         * @param epsilon The allowed difference per component. Default is k_epsilon6.
         * @return true if all components are approximately equal.
         */
        bool almost_equal(const euler3& other, float epsilon = k_epsilon6) const noexcept;

        /**
         * @brief Returns a normalized version of the angles.
         *
         * Each angle is normalized into the range [-π, π], wrapping values that exceed
         * this range. This is often useful to ensure consistency and prevent issues
         * during interpolation or comparison.
         *
         * @return A new euler3 instance with normalized components.
         */
        euler3 normalized() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [roll, pitch, yaw]
         */
        const float* data() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [roll, pitch, yaw]
         */
        float* data() noexcept;

        /**
         * @brief Returns a string representation "[roll, pitch, yaw]" with specified precision
         */
        string to_string(int precision = 3) const;

    public:
        union
        {
            vec3 vec;
            struct
            {
                float roll;  // rotation around X axis
                float pitch; // rotation around Y axis
                float yaw;   // rotation around Z axis
            };
        };
    };

    static_assert(sizeof(euler3) == 12, "incorrect size");
    static_assert(alignof(euler3) == 4, "incorrect alignment");

    inline constexpr euler3::euler3() noexcept
        : roll(0.0f)
        , pitch(0.0f)
        , yaw(0.0f)
    {
    }

    inline constexpr euler3::euler3(float roll, float pitch, float yaw) noexcept
        : roll(roll)
        , pitch(pitch)
        , yaw(yaw)
    {
    }

    inline constexpr euler3::euler3(const vec3& v) noexcept
        : roll(v.x)
        , pitch(v.y)
        , yaw(v.z)
    {
    }

} // namespace tavros::core::math

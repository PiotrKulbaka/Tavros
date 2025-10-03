#pragma once

#include <tavros/core/math/vec3.hpp>

namespace tavros::math
{
    class quat;

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
        static euler3 from_quat(const quat& q) noexcept;

    public:
        /// @brief Default constructor
        constexpr euler3() noexcept;

        /// @brief Constructs a euler3 from individual components
        constexpr euler3(float roll, float pitch, float yaw) noexcept;

        /// @brief Constructs a euler3 from a vec3
        explicit constexpr euler3(const vec3& v) noexcept;

        /// Accesses a component by index [0..2]. Asserts in debug if index is out of bounds
        constexpr float operator[](size_t index) const noexcept;

        /// Accesses a component by index [0..2]. Asserts in debug if index is out of bounds
        constexpr float& operator[](size_t index) noexcept;

        /// Deleted comparison. Use `almost_equal` instead
        bool operator==(const euler3& other) const = delete;

        /// Deleted comparison. Use `almost_equal` instead
        bool operator!=(const euler3& other) const = delete;

        /// @brief Adds another angles to this one
        constexpr euler3& operator+=(const euler3& other) noexcept;

        /// @brief Subtracts another angles from this one
        constexpr euler3& operator-=(const euler3& other) noexcept;

        /// @brief Multiplies this angles by a scalar
        constexpr euler3& operator*=(float scalar) noexcept;

        /// @brief Returns a pointer to the raw float array [roll, pitch, yaw]
        constexpr const float* data() const noexcept;

        /// @brief Returns a pointer to the raw float array [roll, pitch, yaw]
        constexpr float* data() noexcept;

    public:
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4458)


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

#pragma warning(pop)

    constexpr euler3 operator-(const euler3& e) noexcept;
    constexpr euler3 operator+(const euler3& a, const euler3& b) noexcept;
    constexpr euler3 operator-(const euler3& a, const euler3& b) noexcept;
    constexpr euler3 operator*(const euler3& a, const euler3& b) noexcept;
    constexpr euler3 operator*(const euler3& e, float s) noexcept;
    constexpr euler3 operator*(float s, const euler3& e) noexcept;
    constexpr euler3 operator/(const euler3& a, const euler3& b) noexcept;
    constexpr euler3 operator/(const euler3& e, float s) noexcept;
    constexpr euler3 operator/(float s, const euler3& e) noexcept;

} // namespace tavros::math

#pragma warning(push)
#pragma warning(disable : 4458)
#include <tavros/core/math/euler3.inl>
#pragma warning(pop)


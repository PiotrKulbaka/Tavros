#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string.hpp>

namespace tavros::core::math
{

    /**
     * @brief Three-dimensional vector of floats
     *
     * Provides basic arithmetic operations, dot product,
     * linear interpolation, and direct access to components
     * Can be used for positions, colors (RGB), and general-purpose 3D math
     */
    class vec3
    {
    public:
        /**
         * @brief Default constructor Leaves the contents uninitialized
         */
        constexpr vec3() noexcept = default;

        /**
         * @brief Constructs all components with the same value
         */
        constexpr vec3(float v) noexcept;

        /**
         * @brief Constructs a vec3 from individual components
         */
        constexpr vec3(float x, float y, float z) noexcept;

        /**
         * @brief Default destructor
         */
        ~vec3() noexcept = default;

        /**
         * @brief Accesses a component by index [0..2]
         */
        constexpr float operator[](size_t index) const noexcept;

        /**
         * @brief Accesses a component by index [0..2]
         */
        constexpr float& operator[](size_t index) noexcept;

        /**
         * @brief Adds another vector to this one
         */
        constexpr vec3& operator+=(const vec3& other) noexcept;

        /**
         * @brief Subtracts another vector from this one
         */
        constexpr vec3& operator-=(const vec3& other) noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        constexpr vec3& operator*=(float a) noexcept;

        /**
         * @brief Divides this vector by a scalar
         */
        constexpr vec3& operator/=(float a) noexcept;

        /**
         * @brief Divides this vector component-wise by another
         */
        constexpr vec3& operator/=(const vec3& other) noexcept;

        /**
         * @brief Returns the negative of this vector
         */
        constexpr vec3 operator-() const noexcept;

        /**
         * @brief Adds another vector to this one
         */
        constexpr vec3 operator+(const vec3& other) const noexcept;

        /**
         * @brief Subtracts another vector from this one
         */
        constexpr vec3 operator-(const vec3& other) const noexcept;

        /**
         * @brief Multiplies this vector component-wise by another
         */
        constexpr vec3 operator*(const vec3& other) const noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        constexpr vec3 operator*(float a) const noexcept;

        /**
         * @brief Divides this vector by a scalar
         */
        constexpr vec3 operator/(float a) const noexcept;

        /**
         * @brief Equality comparison between two vectors
         */
        constexpr bool operator==(const vec3& other) const noexcept;

        /**
         * @brief Inequality comparison between two vectors
         */
        constexpr bool operator!=(const vec3& other) const noexcept;

        /**
         * @brief Dot product of two vectors
         */
        constexpr float dot(const vec3& other) const noexcept;

        /**
         * @brief Linear interpolation between this and another vector
         * @param target The target vector
         * @param coef The interpolation coefficient [0..1], but can be outside this range, not clamped
         * @return The interpolated vector
         */
        constexpr vec3 lerp(const vec3& target, float coef) const noexcept;

        /**
         * @brief Returns the length of the vector
         */
        float length() const noexcept;

        /**
         * @brief Returns the normalized vector
         */
        vec3 normalized() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array
         */
        constexpr const float* ptr() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array
         */
        constexpr float* ptr() noexcept;

        /**
         * @brief Returns a string representation "(x, y, z)"
         */
        string to_string(int precision = 3) const;

    public:
        union
        {
            struct
            {
                float x, y, z;
            };
            struct
            {
                float r, g, b;
            };
        };
    }; // class vec3

    static_assert(sizeof(vec3) == 12, "incorreect size");
    static_assert(alignof(vec3) == 4, "incorreect alignment");

} // namespace tavros::core::math

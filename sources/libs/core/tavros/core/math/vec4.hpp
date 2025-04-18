#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string.hpp>

namespace tavros::core::math
{

    /**
     * @brief Four-dimensional vector of floats
     *
     * Provides basic arithmetic operations, dot product,
     * linear interpolation, and direct access to components
     * Can be used for positions, colors (RGBA), and general-purpose 4D math
     */
    class alignas(16) vec4
    {
    public:
        /**
         * @brief Default constructor Leaves the contents uninitialized
         */
        constexpr vec4() noexcept = default;

        /**
         * @brief Constructs all components with the same value
         */
        constexpr vec4(float v) noexcept;

        /**
         * @brief Constructs a vec4 from individual components
         */
        constexpr vec4(float x, float y, float z, float w) noexcept;

        /**
         * @brief Default destructor
         */
        ~vec4() noexcept = default;

        /**
         * @brief Accesses a component by index [0..3]
         */
        constexpr float operator[](size_t index) const noexcept;

        /**
         * @brief Accesses a component by index [0..3]
         */
        constexpr float& operator[](size_t index) noexcept;

        /**
         * @brief Adds another vector to this one
         */
        constexpr vec4& operator+=(const vec4& other) noexcept;

        /**
         * @brief Subtracts another vector from this one
         */
        constexpr vec4& operator-=(const vec4& other) noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        constexpr vec4& operator*=(float a) noexcept;

        /**
         * @brief Divides this vector by a scalar
         */
        constexpr vec4& operator/=(float a) noexcept;

        /**
         * @brief Divides this vector component-wise by another
         */
        constexpr vec4& operator/=(const vec4& other) noexcept;

        /**
         * @brief Returns the negative of this vector
         */
        constexpr vec4 operator-() const noexcept;

        /**
         * @brief Adds another vector to this one
         */
        constexpr vec4 operator+(const vec4& other) const noexcept;

        /**
         * @brief Subtracts another vector from this one
         */
        constexpr vec4 operator-(const vec4& other) const noexcept;

        /**
         * @brief Multiplies this vector component-wise by another
         */
        constexpr vec4 operator*(const vec4& other) const noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        constexpr vec4 operator*(float a) const noexcept;

        /**
         * @brief Divides this vector by a scalar
         */
        constexpr vec4 operator/(float a) const noexcept;

        /**
         * @brief Equality comparison between two vectors
         */
        constexpr bool operator==(const vec4& other) const noexcept;

        /**
         * @brief Inequality comparison between two vectors
         */
        constexpr bool operator!=(const vec4& other) const noexcept;

        /**
         * @brief Dot product of two vectors
         */
        constexpr float dot(const vec4& other) const noexcept;

        /**
         * @brief Linear interpolation between this and another vector
         * @param target The target vector
         * @param coef The interpolation coefficient [0..1], but can be outside this range, not clamped
         * @return The interpolated vector
         */
        constexpr vec4 lerp(const vec4& target, float coef) const noexcept;

        /**
         * @brief Returns the length of the vector
         */
        float length() const noexcept;

        /**
         * @brief Returns the normalized vector
         */
        vec4 normalized() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array
         */
        constexpr const float* ptr() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array
         */
        constexpr float* ptr() noexcept;

        /**
         * @brief Returns a string representation "(x, y, z, w)"
         */
        string to_string(int precision = 3) const;

    public:
        union
        {
            struct
            {
                float x, y, z, w;
            };
            struct
            {
                float r, g, b, a;
            };
        };
    }; // class vec4

    static_assert(sizeof(vec4) == 16, "incorreect size");
    static_assert(alignof(vec4) == 16, "incorreect alignment");

} // namespace tavros::core::math

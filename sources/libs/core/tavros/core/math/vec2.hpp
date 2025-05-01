#pragma once

#include <tavros/core/math/functions/basic_math.hpp>

namespace tavros::math
{

    /**
     * @brief Two-dimensional vector of floats
     *
     * Provides basic arithmetic operations, dot product,
     * linear interpolation, and direct access to components
     * Can be used for positions, and general-purpose 2D math
     */
    class vec2
    {
    public:
        /// @brief Default constructor, constructs a zero vector
        constexpr vec2() noexcept;

        /// @brief Constructs all components with the same value
        constexpr vec2(float v) noexcept;

        /// @brief Constructs a vec2 from individual components
        constexpr vec2(float x, float y) noexcept;

        /// @brief Accesses a component by index [0..1]. Asserts in debug if index is out of bounds
        constexpr float operator[](size_t index) const noexcept;

        /// @brief Accesses a component by index [0..1]. Asserts in debug if index is out of bounds
        constexpr float& operator[](size_t index) noexcept;

        /// @brief Deleted comparison. Use `almost_equal` instead
        bool operator==(const vec2& other) const = delete;

        /// @brief Deleted comparison. Use `almost_equal` instead
        bool operator!=(const vec2& other) const = delete;

        /// @brief Adds another vector to this one
        constexpr vec2& operator+=(const vec2& other) noexcept;

        /// @brief Subtracts another vector from this one
        constexpr vec2& operator-=(const vec2& other) noexcept;

        /// @brief Multiplies this vector by a scalar
        constexpr vec2& operator*=(float scalar) noexcept;

        /// @brief Divides this vector by a scalar. Asserts in debug if `scalar` is zero
        constexpr vec2& operator/=(float scalar) noexcept;

        /// @brief Divides this vector component-wise by another. Asserts in debug if any component of `other` is zero
        constexpr vec2& operator/=(const vec2& other) noexcept;

        /// @brief Returns a pointer to the raw float array [x, y]
        constexpr const float* data() const noexcept;

        /// @brief Returns a pointer to the raw float array [x, y]
        constexpr float* data() noexcept;

    public:
        union
        {
            struct
            {
                float x, y;
            };
            struct
            {
                float u, v;
            };
            struct
            {
                float width, height;
            };
        };
    };

    using point2 = vec2;
    using size2 = vec2;


    constexpr vec2 operator-(const vec2& v) noexcept;
    constexpr vec2 operator+(const vec2& a, const vec2& b) noexcept;
    constexpr vec2 operator-(const vec2& a, const vec2& b) noexcept;
    constexpr vec2 operator*(const vec2& a, const vec2& b) noexcept;
    constexpr vec2 operator*(const vec2& v, float s) noexcept;
    constexpr vec2 operator*(float s, const vec2& v) noexcept;
    constexpr vec2 operator/(const vec2& a, const vec2& b) noexcept;
    constexpr vec2 operator/(const vec2& v, float s) noexcept;
    constexpr vec2 operator/(float s, const vec2& v) noexcept;

} // namespace tavros::math

#include <tavros/core/math/vec2.inl>

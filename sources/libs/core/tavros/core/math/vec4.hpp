#pragma once

#include <tavros/core/math/vec3.hpp>

namespace tavros::math
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
        /// @brief Default constructor, constructs a zero vector
        constexpr vec4() noexcept;

        /// @brief Constructs all components with the same value
        constexpr vec4(float v) noexcept;

        /// @brief Constructs a vec4 from individual components
        constexpr vec4(float x, float y, float z, float w) noexcept;

        /// @brief Accesses a component by index [0..3]. Asserts in debug if index is out of bounds
        constexpr float operator[](size_t index) const noexcept;

        /// @brief Accesses a component by index [0..3]. Asserts in debug if index is out of bounds
        constexpr float& operator[](size_t index) noexcept;

        /// @brief Deleted comparison. Use `almost_equal` instead
        bool operator==(const vec4& other) const = delete;

        /// @brief Deleted comparison. Use `almost_equal` instead
        bool operator!=(const vec4& other) const = delete;

        /// @brief Adds another vector to this one
        constexpr vec4& operator+=(const vec4& other) noexcept;

        /// @brief Subtracts another vector from this one
        constexpr vec4& operator-=(const vec4& other) noexcept;

        /// @brief Multiplies this vector by a scalar
        constexpr vec4& operator*=(float scalar) noexcept;

        /// @brief Divides this vector by a scalar. Asserts in debug if `scalar` is zero
        constexpr vec4& operator/=(float scalar) noexcept;

        /// @brief Divides this vector component-wise by another. Asserts in debug if any component of `other` is zero
        constexpr vec4& operator/=(const vec4& other) noexcept;

        /// @brief Sets the components of the vector to the specified values
        constexpr void set(float x, float y, float z, float w) noexcept;

        /// @brief Returns a pointer to the raw float array [x, y, z, w]
        constexpr const float* data() const noexcept;

        /// @brief Returns a pointer to the raw float array [x, y, z, w]
        constexpr float* data() noexcept;

    public:
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4458)

        union
        {
            vec3 xyz;
            struct
            {
                float x, y, z, w;
            };
            struct
            {
                float r, g, b, a;
            };
        };
    };

#pragma warning(pop)

    constexpr vec4 operator-(const vec4& v) noexcept;
    constexpr vec4 operator+(const vec4& a, const vec4& b) noexcept;
    constexpr vec4 operator-(const vec4& a, const vec4& b) noexcept;
    constexpr vec4 operator*(const vec4& a, const vec4& b) noexcept;
    constexpr vec4 operator*(const vec4& v, float s) noexcept;
    constexpr vec4 operator*(float s, const vec4& v) noexcept;
    constexpr vec4 operator/(const vec4& a, const vec4& b) noexcept;
    constexpr vec4 operator/(const vec4& v, float s) noexcept;
    constexpr vec4 operator/(float s, const vec4& v) noexcept;

} // namespace tavros::math

#pragma warning(push)
#pragma warning(disable : 4458)
#include <tavros/core/math/vec4.inl>
#pragma warning(pop)


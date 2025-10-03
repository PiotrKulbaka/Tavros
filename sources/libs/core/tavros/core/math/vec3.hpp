#pragma once

#include <tavros/core/math/vec2.hpp>

namespace tavros::math
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
        /// @brief Default constructor, constructs a zero vector
        constexpr vec3() noexcept;

        /// @brief Constructs all components with the same value
        constexpr vec3(float v) noexcept;

        /// @brief Constructs a vec3 from individual components
        constexpr vec3(float x, float y, float z) noexcept;

        /// Accesses a component by index [0..2]. Asserts in debug if index is out of bounds
        constexpr float operator[](size_t index) const noexcept;

        /// Accesses a component by index [0..2]. Asserts in debug if index is out of bounds
        constexpr float& operator[](size_t index) noexcept;

        /// Deleted comparison. Use `almost_equal` instead
        bool operator==(const vec3& other) const = delete;

        /// Deleted comparison. Use `almost_equal` instead
        bool operator!=(const vec3& other) const = delete;

        /// @brief Adds another vector to this one
        constexpr vec3& operator+=(const vec3& other) noexcept;

        /// @brief Subtracts another vector from this one
        constexpr vec3& operator-=(const vec3& other) noexcept;

        /// @brief Multiplies this vector by a scalar
        constexpr vec3& operator*=(float scalar) noexcept;

        /// @brief Divides this vector by a scalar. Asserts in debug if `scalar` is zero
        constexpr vec3& operator/=(float scalar) noexcept;

        /// @brief Divides this vector component-wise by another. Asserts in debug if any component of `other` is zero
        constexpr vec3& operator/=(const vec3& other) noexcept;

        /// @brief Sets the components of the vector to the specified values
        constexpr void set(float x, float y, float z) noexcept;

        /// @brief Returns a pointer to the raw float array [x, y, z]
        constexpr const float* data() const noexcept;

        /// @brief Returns a pointer to the raw float array [x, y, z]
        constexpr float* data() noexcept;

    public:
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4458)

        union
        {
            vec2 xy;
            struct
            {
                float x, y, z;
            };
            struct
            {
                float r, g, b;
            };
            struct
            {
                float u, v, d;
            };
        };
    };

#pragma warning(pop)

    constexpr vec3 operator-(const vec3& v) noexcept;
    constexpr vec3 operator+(const vec3& a, const vec3& b) noexcept;
    constexpr vec3 operator-(const vec3& a, const vec3& b) noexcept;
    constexpr vec3 operator*(const vec3& a, const vec3& b) noexcept;
    constexpr vec3 operator*(const vec3& v, float s) noexcept;
    constexpr vec3 operator*(float s, const vec3& v) noexcept;
    constexpr vec3 operator/(const vec3& a, const vec3& b) noexcept;
    constexpr vec3 operator/(const vec3& v, float s) noexcept;
    constexpr vec3 operator/(float s, const vec3& v) noexcept;

} // namespace tavros::math

#pragma warning(push)
#pragma warning(disable : 4458)
#include <tavros/core/math/vec3.inl>
#pragma warning(pop)


#pragma once

#include <tavros/core/types.hpp>

namespace tavros::math
{

    /**
     * @brief Two-dimensional vector of int32
     *
     * Provides basic arithmetic operations, and direct access to components
     */
    class ivec2
    {
    public:
        /// @brief Default constructor, constructs a zero vector
        constexpr ivec2() noexcept;

        /// @brief Constructs a ivec2 from individual components
        constexpr ivec2(int32 x, int32 y) noexcept;

        /// @brief Adds another vector to this one
        constexpr ivec2& operator+=(const ivec2& other) noexcept;

        /// @brief Subtracts another vector from this one
        constexpr ivec2& operator-=(const ivec2& other) noexcept;

        /// @brief Multiplies this vector component-wise by another
        constexpr ivec2& operator*=(const ivec2& other) noexcept;

        /// @brief Multiplies this vector by a scalar
        constexpr ivec2& operator*=(int32 scalar) noexcept;

        /// @brief Divides this vector component-wise by another. Asserts in debug if any component of `other` is zero
        constexpr ivec2& operator/=(const ivec2& other) noexcept;

        /// @brief Divides this vector by a scalar. Asserts in debug if `scalar` is zero
        constexpr ivec2& operator/=(int32 scalar) noexcept;

    public:
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4458)

        union
        {
            struct
            {
                int32 x, y;
            };
            struct
            {
                int32 left, top;
            };
            struct
            {
                int32 width, height;
            };
        };
    };

#pragma warning(pop)

    using ipoint2 = ivec2;
    using isize2 = ivec2;

    constexpr ivec2 operator-(const ivec2& v) noexcept;
    constexpr ivec2 operator+(const ivec2& a, const ivec2& b) noexcept;
    constexpr ivec2 operator-(const ivec2& a, const ivec2& b) noexcept;
    constexpr ivec2 operator*(const ivec2& a, const ivec2& b) noexcept;
    constexpr ivec2 operator*(const ivec2& v, int32 s) noexcept;
    constexpr ivec2 operator*(int32 s, const ivec2& v) noexcept;
    constexpr ivec2 operator/(const ivec2& a, const ivec2& b) noexcept;
    constexpr ivec2 operator/(const ivec2& v, int32 s) noexcept;
    constexpr ivec2 operator/(int32 s, const ivec2& v) noexcept;

} // namespace tavros::math

#pragma warning(push)
#pragma warning(disable : 4458)
#include <tavros/core/math/ivec2.inl>
#pragma warning(pop)


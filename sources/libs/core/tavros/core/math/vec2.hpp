#pragma once

#include <tavros/core/math/base_math.hpp>
#include <tavros/core/types.hpp>
#include <tavros/core/string.hpp>

namespace tavros::core::math
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
        /**
         * @brief Default constructor. Leaves the contents uninitialized
         */
        constexpr vec2() noexcept = default;

        /**
         * @brief Constructs all components with the same value
         */
        constexpr vec2(float v) noexcept;

        /**
         * @brief Constructs a vec2 from individual components
         */
        constexpr vec2(float x, float y) noexcept;

        /**
         * @brief Default destructor
         */
        ~vec2() noexcept = default;

        /**
         * @brief Accesses a component by index [0..1]
         * @note Asserts in debug if index is out of bounds
         */
        float operator[](size_t index) const noexcept;

        /**
         * @brief Accesses a component by index [0..1]
         * @note Asserts in debug if index is out of bounds
         */
        float& operator[](size_t index) noexcept;

        /**
         * @brief Adds another vector to this one
         */
        vec2& operator+=(const vec2& other) noexcept;

        /**
         * @brief Subtracts another vector from this one
         */
        vec2& operator-=(const vec2& other) noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        vec2& operator*=(float a) noexcept;

        /**
         * @brief Divides this vector by a scalar
         * @note Asserts in debug if `a` is zero
         */
        vec2& operator/=(float a) noexcept;

        /**
         * @brief Divides this vector component-wise by another
         * @note Asserts in debug if any component of `other` is zero
         */
        vec2& operator/=(const vec2& other) noexcept;

        /**
         * @brief Returns the negative of this vector
         */
        vec2 operator-() const noexcept;

        /**
         * @brief Adds another vector to this one
         */
        vec2 operator+(const vec2& other) const noexcept;

        /**
         * @brief Subtracts another vector from this one
         */
        vec2 operator-(const vec2& other) const noexcept;

        /**
         * @brief Multiplies this vector component-wise by another
         */
        vec2 operator*(const vec2& other) const noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        vec2 operator*(float a) const noexcept;

        /**
         * @brief Divides this vector by a scalar
         * @note Asserts in debug mode if `a` is zero
         */
        vec2 operator/(float a) const noexcept;

        /**
         * @brief Equality comparison between two vectors
         */
        bool operator==(const vec2& other) const noexcept;

        /**
         * @brief Inequality comparison between two vectors
         */
        bool operator!=(const vec2& other) const noexcept;

        /**
         * @brief Equality comparison between two vectors with a epsilon tolerance
         */
        bool almost_equal(const vec2& other, float epsilon = k_vec_compare_epsilon) const noexcept;

        /**
         * @brief 2D cross product (returns signed area / orientation)
         *
         * Computes the scalar cross product (also known as the perp-dot product) of this 2D vector and another
         * The result represents the signed area of the parallelogram formed by the two vectors,
         * and can be used to determine the relative orientation between them.
         *
         * - Positive result -> `other` is counter-clockwise (to the left) from `this`
         * - Negative result -> `other` is clockwise (to the right) from `this`
         * - Zero -> the vectors are collinear
         *
         * This is commonly used in:
         * - Determining turn direction (left/right) between vectors
         * - Computing 2D triangle area: `0.5f * abs(a.cross(b))`
         * - Checking orientation in convex hull or intersection tests
         *
         * @param other The other 2D vector
         * @return Signed scalar representing the 2D cross product
         */
        float cross(const vec2& other) const noexcept;

        /**
         * @brief Dot product of two vectors
         *
         * - Positive result -> vectors point in a similar direction
         * - Zero -> vectors are orthogonal (perpendicular)
         * - Negative result -> vectors point in opposite directions
         *
         * This is commonly used in:
         * - Calculating the cosine of the angle between two vectors
         * - Calculating angles between vectors
         * - Projecting one vector onto another
         * - Determining alignment in lighting or physics calculations
         *
         * @param other The other 2D vector
         * @return Scalar value representing the dot product
         */
        float dot(const vec2& other) const noexcept;

        /**
         * @brief Linear interpolation between this and another vector
         * @param target The target vector
         * @param coef The interpolation coefficient [0..1], but can be outside this range, not clamped
         * @return The interpolated vector
         */
        vec2 lerp(const vec2& target, float coef) const noexcept;

        /**
         * @brief Returns the length of the vector
         */
        float length() const noexcept;

        /**
         * @brief Returns the normalized vector
         */
        vec2 normalized() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [x, y]
         */
        const float* data() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [x, y]
         */
        float* data() noexcept;

        /**
         * @brief Returns a string representation "[x, y]" with specified precision
         */
        string to_string(int precision = 3) const;

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
        };
    }; // class vec2

    static_assert(sizeof(vec2) == 8, "incorrect size");
    static_assert(alignof(vec2) == 4, "incorrect alignment");

    inline constexpr vec2::vec2(float v) noexcept
        : x(v)
        , y(v)
    {
    }

    inline constexpr vec2::vec2(float x, float y) noexcept
        : x(x)
        , y(y)
    {
    }

} // namespace tavros::core::math

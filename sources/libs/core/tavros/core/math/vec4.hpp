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
        /**
         * @brief Default constructor, constructs a zero vector
         */
        constexpr vec4() noexcept;

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
         * @note Asserts in debug if index is out of bounds
         */
        float operator[](size_t index) const noexcept;

        /**
         * @brief Accesses a component by index [0..3]
         * @note Asserts in debug if index is out of bounds
         */
        float& operator[](size_t index) noexcept;

        /**
         * @brief Adds another vector to this one
         */
        vec4& operator+=(const vec4& other) noexcept;

        /**
         * @brief Subtracts another vector from this one
         */
        vec4& operator-=(const vec4& other) noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        vec4& operator*=(float scalar) noexcept;

        /**
         * @brief Divides this vector by a scalar
         * @note Asserts in debug if `a` is zero
         */
        vec4& operator/=(float scalar) noexcept;

        /**
         * @brief Divides this vector component-wise by another
         * @note Asserts in debug if any component of `other` is zero
         */
        vec4& operator/=(const vec4& other) noexcept;

        /**
         * @brief Returns the negative of this vector
         */
        vec4 operator-() const noexcept;

        /**
         * @brief Adds another vector to this one
         */
        vec4 operator+(const vec4& other) const noexcept;

        /**
         * @brief Subtracts another vector from this one
         */
        vec4 operator-(const vec4& other) const noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        vec4 operator*(float scalar) const noexcept;

        /**
         * @brief Multiplies this vector component-wise by another
         */
        vec4 operator*(const vec4& other) const noexcept;

        /**
         * @brief Divides this vector by a scalar
         * @note Asserts in debug if `a` is zero
         */
        vec4 operator/(float scalar) const noexcept;

        /**
         * @brief Deleted comparison. Use `almost_equal` instead
         */
        bool operator==(const vec4& other) const = delete;

        /**
         * @brief Deleted comparison. Use `almost_equal` instead
         */
        bool operator!=(const vec4& other) const = delete;

        /**
         * @brief Compares two sets of vec4 with a given tolerance.
         *
         * Returns true if the absolute difference between corresponding components
         * of the two vec4 sets is less than or equal to the specified epsilon.
         *
         * This is useful for floating-point comparisons where exact equality is
         * not reliable.
         *
         * @param other The other vec4 instance to compare with.
         * @param epsilon The allowed difference per component. Default is k_epsilon6.
         * @return true if all components are approximately equal.
         */
        bool almost_equal(const vec4& other, float epsilon = k_epsilon6) const noexcept;

        /**
         * @brief Dot product of two 4D vectors
         *
         * Computes the dot product (scalar product) between this 4D vector and another
         * and it measures how aligned the two vectors are in 4D space.
         *
         * - Positive result -> vectors point in a similar direction
         * - Zero -> vectors are orthogonal (perpendicular)
         * - Negative result -> vectors point in opposite directions
         *
         * Commonly used in:
         * - Calculating the cosine of the angle between two vectors
         * - Calculating normals for lighting and geometry
         * - Calculating angles between vectors (e.g. for lighting)
         * - Projecting one vector onto another
         * - Determining alignment in physics, shading, and geometry
         *
         * @param other The other 4D vector
         * @return Dot product of two vectors
         */
        float dot(const vec4& other) const noexcept;

        /**
         * @brief Linear interpolation between this and another vector
         * @param target The target vector
         * @param coef The interpolation coefficient [0..1], but can be outside this range, not clamped
         * @return The interpolated vector
         */
        vec4 lerp(const vec4& target, float coef) const noexcept;

        /**
         * @brief Returns the length of the vector
         */
        float length() const noexcept;

        /**
         * @brief Returns the normalized vector.
         *
         * @note Returns normalized vector. If the vector has zero length, this method returns vec4(0, 0, 0, 1)
         * @note Asserts in debug if the vector has zero length
         */
        vec4 normalized() const noexcept;

        /**
         * @brief Returns the minimum of this and another vector component-wise
         */
        vec4 min(const vec4& other) const noexcept;

        /**
         * @brief Returns the maximum of this and another vector component-wise
         */
        vec4 max(const vec4& other) const noexcept;

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
    }; // class vec4

    static_assert(sizeof(vec4) == 16, "incorrect size");
    static_assert(alignof(vec4) == 16, "incorrect alignment");

    inline constexpr vec4::vec4() noexcept
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
        , w(0.0f)
    {
    }

    inline constexpr vec4::vec4(float v) noexcept
        : x(v)
        , y(v)
        , z(v)
        , w(v)
    {
    }

    inline constexpr vec4::vec4(float x, float y, float z, float w) noexcept
        : x(x)
        , y(y)
        , z(z)
        , w(w)
    {
    }

} // namespace tavros::math

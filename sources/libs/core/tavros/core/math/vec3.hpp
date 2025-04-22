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
        /**
         * @brief Default constructor, constructs a zero vector
         */
        constexpr vec3() noexcept;

        /**
         * @brief Constructs all components with the same value
         */
        constexpr vec3(float v) noexcept;

        /**
         * @brief Constructs a vec3 from individual components
         */
        constexpr vec3(float x, float y, float z) noexcept;

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
         * @brief Adds another vector to this one
         */
        vec3& operator+=(const vec3& other) noexcept;

        /**
         * @brief Subtracts another vector from this one
         */
        vec3& operator-=(const vec3& other) noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        vec3& operator*=(float scalar) noexcept;

        /**
         * @brief Divides this vector by a scalar
         * @note Asserts in debug if `a` is zero
         */
        vec3& operator/=(float scalar) noexcept;

        /**
         * @brief Divides this vector component-wise by another
         * @note Asserts in debug if any component of `other` is zero
         */
        vec3& operator/=(const vec3& other) noexcept;

        /**
         * @brief Returns the negative of this vector
         */
        vec3 operator-() const noexcept;

        /**
         * @brief Adds another vector to this one
         */
        vec3 operator+(const vec3& other) const noexcept;

        /**
         * @brief Subtracts another vector from this one
         */
        vec3 operator-(const vec3& other) const noexcept;

        /**
         * @brief Multiplies this vector by a scalar
         */
        vec3 operator*(float scalar) const noexcept;

        /**
         * @brief Multiplies this vector component-wise by another
         */
        vec3 operator*(const vec3& other) const noexcept;

        /**
         * @brief Divides this vector by a scalar
         * @note Asserts in debug if `a` is zero
         */
        vec3 operator/(float scalar) const noexcept;

        /**
         * @brief Deleted comparison. Use `almost_equal` instead
         */
        bool operator==(const vec3& other) const = delete;

        /**
         * @brief Deleted comparison. Use `almost_equal` instead
         */
        bool operator!=(const vec3& other) const = delete;

        /**
         * @brief Compares two sets of vec3 with a given tolerance.
         *
         * Returns true if the absolute difference between corresponding components
         * of the two vec3 sets is less than or equal to the specified epsilon.
         *
         * This is useful for floating-point comparisons where exact equality is
         * not reliable.
         *
         * @param other The other vec3 instance to compare with.
         * @param epsilon The allowed difference per component. Default is k_epsilon6.
         * @return true if all components are approximately equal.
         */
        bool almost_equal(const vec3& other, float epsilon = k_epsilon6) const noexcept;

        /**
         * @brief Calculates the angle (in radians) between this vector and another vector.
         *
         * The angle is computed using the formula:
         *      θ = acos((A · B) / (|A| * |B|))
         *
         * @note In the case where either of the vectors has zero length, the angle
         *       is defined as 0 radians (k_epsilon6 is used for comparisons)
         *
         * @note In debug, an assertion will be triggered if either of the vectors has zero length.
         *
         * @param other The other vector to compare this vector with.
         * @return The angle between the two vectors in radians.
         */
        float angle(const vec3& other) const noexcept;

        /**
         * @brief 3D cross product (returns a vector perpendicular to both)
         *
         * Computes the vector cross product of this 3D vector and another.
         * The result is a new vector that is perpendicular to the plane defined by the two inputs,
         * with a direction determined by the right-hand rule and a magnitude equal to the area
         * of the parallelogram spanned by the vectors.
         *
         * This implementation assumes a **left-handed coordinate system** (X forward, Y right, Z up).
         * If you're using a **right-handed coordinate system**, you must manually negate the result
         * (i.e., multiply by -1).
         *
         * This is commonly used in:
         * - Calculating normals for lighting and geometry
         * - Constructing orthogonal bases
         * - Determining orientation and rotation in 3D space
         * - Physics (e.g., torque and angular momentum)
         *
         * Note: If the input vectors are parallel or collinear, the result is the zero vector.
         *
         * @param other The other 3D vector
         * @return A new vector representing the cross product
         */
        vec3 cross(const vec3& other) const noexcept;

        /**
         * @brief Dot product of two 3D vectors
         *
         * Computes the dot product (scalar product) between this 3D vector and another
         * and it measures how aligned the two vectors are in 3D space.
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
         * @param other The other 3D vector
         * @return Scalar value representing the dot product
         */
        float dot(const vec3& other) const noexcept;

        /**
         * @brief Linear interpolation between this and another vector
         * @param target The target vector
         * @param coef The interpolation coefficient [0..1], but can be outside this range, not clamped
         * @return The interpolated vector
         */
        vec3 lerp(const vec3& target, float coef) const noexcept;

        /**
         * @brief Returns the length of the vector
         */
        float length() const noexcept;

        /**
         * @brief Returns the normalized vector
         *
         * @note Returns normalized vector. If the vector has zero length, this method returns vec4(0, 0, 1)
         * @note Asserts in debug if the vector has zero length
         */
        vec3 normalized() const noexcept;

        /**
         * @brief Returns an arbitrary vector orthogonal to this one.
         *
         * The result is not normalized. The smallest component is selected to minimize precision loss.
         * If this vector is zero, the result is undefined.
         *
         * @return A vector perpendicular to this one.
         */
        vec3 orthogonal() const noexcept;

        /**
         * @brief Returns the minimum of this and another vector component-wise
         */
        vec3 min(const vec3& other) const noexcept;

        /**
         * @brief Returns the maximum of this and another vector component-wise
         */
        vec3 max(const vec3& other) const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [x, y, z]
         */
        const float* data() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [x, y, z]
         */
        float* data() noexcept;

        /**
         * @brief Returns a string representation "[x, y, z]" with specified precision
         */
        core::string to_string(int precision = 3) const;

    public:
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
    }; // class vec3

    static_assert(sizeof(vec3) == 12, "incorrect size");
    static_assert(alignof(vec3) == 4, "incorrect alignment");

    inline constexpr vec3::vec3() noexcept
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
    {
    }

    inline constexpr vec3::vec3(float v) noexcept
        : x(v)
        , y(v)
        , z(v)
    {
    }

    inline constexpr vec3::vec3(float x, float y, float z) noexcept
        : x(x)
        , y(y)
        , z(z)
    {
    }

} // namespace tavros::math

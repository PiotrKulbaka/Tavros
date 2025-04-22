#pragma once

#include <tavros/core/math/vec4.hpp>

namespace tavros::core::math
{

    /**
     * @brief Represents a 4x4 matrix of 32-bit floating point values in **column-major** layout.
     *
     * Each column is stored as a `vec4`, allowing efficient mathematical operations such as
     * matrix-vector and matrix-matrix multiplication. This matches the column-major format used
     * in OpenGL and GLSL.
     *
     * It is particularly suited for transformations in 3D space, including:
     * - Translation
     * - Rotation
     * - Scaling
     * - Perspective and orthographic projection
     * - View transformations
     *
     * Performance Considerations:
     * - Most operations are marked `noexcept` where possible.
     * - Assumes use in real-time rendering or simulations where allocation and exception-handling must be avoided.
     *
     * @note
     * - `operator[]` provides access to columns (not rows).
     *
     * @see vec4
     */
    class mat4
    {
    public:
        /**
         * @brief Default constructor, constructs a identity matrix
         */
        constexpr mat4() noexcept;

        /**
         * @brief Construct from 4 columns
         */
        constexpr explicit mat4(const vec4& col0, const vec4& col1, const vec4& col2, const vec4& col3) noexcept;

        /**
         * @brief Construct from scalar (diagonal matrix)
         */
        constexpr explicit mat4(float diag) noexcept;

        /**
         * @brief Accesses a column by index [0..3]
         * @note Asserts in debug if index is out of bounds
         */
        vec4& operator[](size_t i) noexcept;

        /**
         * @brief Accesses a column by index [0..3]
         * @note Asserts in debug if index is out of bounds
         */
        const vec4& operator[](size_t i) const noexcept;

        /**
         * @brief Deleted comparison. Use `almost_equal` instead
         */
        bool operator==(const mat4& m) const = delete;

        /**
         * @brief Deleted comparison. Use `almost_equal` instead
         */
        bool operator!=(const mat4& m) const = delete;

        /**
         * @brief Compares two sets of mat4 with a given tolerance.
         *
         * Returns true if the absolute difference between corresponding components
         * of the two mat4 sets is less than or equal to the specified epsilon.
         *
         * This is useful for floating-point comparisons where exact equality is
         * not reliable.
         *
         * @param other The other mat4 instance to compare with.
         * @param epsilon The allowed difference per component. Default is k_epsilon6.
         * @return true if all components are approximately equal.
         */
        bool almost_equal(const mat4& m, float epsilon = k_epsilon6) const noexcept;

        /**
         * @brief Returns the negated matrix (element-wise negation)
         */
        mat4 operator-() const noexcept;

        /**
         * @brief Multiplies the matrix by a scalar
         */
        mat4 operator*(float scalar) const noexcept;

        /**
         * @brief Multiplies the matrix with a 4D vector
         */
        vec4 operator*(const vec4& v) const noexcept;

        /**
         * @brief Matrix multiplication
         */
        mat4 operator*(const mat4& m) const noexcept;

        /**
         * @brief Adds two matrices element-wise
         */
        mat4 operator+(const mat4& m) const noexcept;

        /**
         * @brief Subtracts two matrices element-wise
         */
        mat4 operator-(const mat4& m) const noexcept;

        /**
         * @brief In-place scalar multiplication
         */
        mat4& operator*=(const float scalar) noexcept;

        /**
         * @brief In-place matrix multiplication
         */
        mat4& operator*=(const mat4& m) noexcept;

        /**
         * @brief In-place element-wise addition
         */
        mat4& operator+=(const mat4& m) noexcept;

        /**
         * @brief In-place element-wise subtraction
         */
        mat4& operator-=(const mat4& m) noexcept;

        /**
         * @brief Transpose matrix
         */
        mat4 transpose() const noexcept;

        /**
         * @brief Computes the determinant of the matrix.
         *
         * The determinant is a scalar value that encodes certain properties of the matrix:
         * - If the determinant is zero, the matrix is **singular** and **not invertible**.
         * - A non-zero determinant implies that the matrix can be inverted and preserves volume (up to scaling).
         *
         * This function is commonly used as a prerequisite check before attempting matrix inversion.
         *
         * @note This operation involves multiple multiplications and additions; it is relatively expensive.
         *
         * @return A floating-point scalar representing the determinant of the matrix.
         */
        float determinant() const noexcept;

        /**
         * @brief Computes and returns the inverse of this matrix.
         *
         * Matrix inversion is used to reverse transformations or change between coordinate spaces
         * (e.g., world-space to view-space). This method performs a full inverse of the 4x4 matrix.
         *
         * Internally, the function uses the adjugate and determinant method:
         * - If the determinant is zero, the matrix is not invertible and the returned result is zero matrix.
         * - It is the caller's responsibility to check for invertibility using `determinant()` if safety is required.
         *
         * @warning Zero matrix is returned if the matrix is singular (determinant == 0)
         * @note This operation involves multiple multiplications and additions; it is relatively expensive.
         *
         * @return The inverse of this matrix.
         * @see determinant()
         */
        mat4 inverse() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [col1, col2, col3, col4]
         */
        const float* data() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [col1, col2, col3, col4]
         */
        float* data() noexcept;

        /**
         * @brief Returns a string representation "[[c1.x, ..., c1.w], ..., [c4.x, ..., c4.w]]" with specified precision
         */
        string to_string(int precision = 3) const;

    public:
        /**
         * @brief Returns the identity matrix
         */
        static constexpr mat4 identity() noexcept;

    public:
        union
        {
            struct
            {
                vec4 cols[4];
            };
            struct
            {
                vec4 col0, col1, col2, col3;
            };
        };
    };

    static_assert(sizeof(mat4) == 64, "incorrect size");
    static_assert(alignof(mat4) == 16, "incorrect alignment");

    inline constexpr mat4::mat4() noexcept
        : mat4(1.0f)
    {
    }

    inline constexpr mat4::mat4(const vec4& col0, const vec4& col1, const vec4& col2, const vec4& col3) noexcept
        : cols{col0, col1, col2, col3}
    {
    }

    inline constexpr mat4::mat4(float diag) noexcept
        : cols{
              vec4(diag, 0.0f, 0.0f, 0.0f),
              vec4(0.0f, diag, 0.0f, 0.0f),
              vec4(0.0f, 0.0f, diag, 0.0f),
              vec4(0.0f, 0.0f, 0.0f, diag)
          }
    {
    }

    inline constexpr mat4 mat4::identity() noexcept
    {
        return mat4(1.0f);
    }

} // namespace tavros::core::math

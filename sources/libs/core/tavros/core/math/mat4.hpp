#pragma once

#include <tavros/core/math/vec4.hpp>

#include <tavros/core/types.hpp>
#include <tavros/core/string.hpp>

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
     * - Default constructor does **not** initialize memory.
     * - `operator[]` provides access to columns (not rows).
     * - Matrix inversion is undefined if the matrix is not invertible (determinant is zero).
     *
     * @see vec4
     */
    class mat4
    {
    public:
        /**
         * @brief Default constructor. Leaves the contents uninitialized
         */
        constexpr mat4() noexcept = default;

        /**
         * @brief Construct from 4 columns
         */
        constexpr explicit mat4(const vec4& col0, const vec4& col1, const vec4& col2, const vec4& col3) noexcept;

        /**
         * @brief Construct from scalar (diagonal matrix)
         */
        constexpr explicit mat4(float diag) noexcept;

        /**
         * @brief Accesses a row by index [0..3]
         * @note Asserts in debug if index is out of bounds
         */
        vec4& operator[](size_t i) noexcept;

        /**
         * @brief Accesses a row by index [0..3]
         * @note Asserts in debug if index is out of bounds
         */
        const vec4& operator[](size_t i) const noexcept;

        /**
         * @brief Equality comparison between two matrices
         */
        bool operator==(const mat4& m) const noexcept;

        /**
         * @brief Inequality comparison between two matrices
         */
        bool operator!=(const mat4& m) const noexcept;

        /**
         * @brief Equality comparison between two matrices with a epsilon tolerance
         */
        bool almost_equal(const mat4& m, float epsilon = 1e-6f) const noexcept;

        /**
         * @brief Returns the negated matrix (element-wise negation)
         */
        mat4 operator-() const noexcept;

        /**
         * @brief Multiplies the matrix by a scalar
         */
        mat4 operator*(float a) const noexcept;

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
        mat4& operator*=(const float a);

        /**
         * @brief In-place matrix multiplication
         */
        mat4& operator*=(const mat4& m);

        /**
         * @brief In-place element-wise addition
         */
        mat4& operator+=(const mat4& m);

        /**
         * @brief In-place element-wise subtraction
         */
        mat4& operator-=(const mat4& m);

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
         * - If the determinant is zero, the matrix is not invertible and the returned result is undefined.
         * - It is the caller's responsibility to check for invertibility using `determinant()` if safety is required.
         *
         * @warning Undefined behavior if the matrix is singular (determinant == 0). No internal checks are performed.
         *
         * @note This operation involves multiple multiplications and additions; it is relatively expensive.
         *
         * @return The inverse of this matrix.
         *
         * @see determinant()
         */
        mat4 inverse() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [col1, col2, col3, col4]
         */
        const float* ptr() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [col1, col2, col3, col4]
         */
        float* ptr() noexcept;

        /**
         * @brief Returns a string representation "[[c1.x, ..., c1.z], ..., [c4.x, ..., c4.z]]" with specified precision
         */
        string to_string(int precision = 3) const;

    public:
        vec4 cols[4];
    };

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


} // namespace tavros::core::math

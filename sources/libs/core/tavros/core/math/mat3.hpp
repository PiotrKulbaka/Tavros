#pragma once

#include <tavros/core/math/vec3.hpp>

#include <tavros/core/types.hpp>
#include <tavros/core/string.hpp>

namespace tavros::core::math
{

    /**
     * @brief Represents a 3x3 matrix of 32-bit floating point values in **column-major** layout.
     *
     * Each column is stored as a `vec3`, allowing efficient mathematical operations such as
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
     * @see vec3
     */
    class mat3
    {
    public:
        /**
         * @brief Default constructor. Leaves the contents uninitialized
         */
        constexpr mat3() noexcept = default;

        /**
         * @brief Construct from 3 columns
         */
        constexpr explicit mat3(const vec3& col0, const vec3& col1, const vec3& col2) noexcept;

        /**
         * @brief Construct from scalar (diagonal matrix)
         */
        constexpr explicit mat3(float diag) noexcept;

        /**
         * @brief Accesses a column by index [0..2]
         * @note Asserts in debug if index is out of bounds
         */
        vec3& operator[](size_t i) noexcept;

        /**
         * @brief Accesses a column by index [0..2]
         * @note Asserts in debug if index is out of bounds
         */
        const vec3& operator[](size_t i) const noexcept;

        /**
         * @brief Equality comparison between two matrices
         */
        bool operator==(const mat3& m) const noexcept;

        /**
         * @brief Inequality comparison between two matrices
         */
        bool operator!=(const mat3& m) const noexcept;

        /**
         * @brief Equality comparison between two matrices with a epsilon tolerance
         */
        bool almost_equal(const mat3& m, float epsilon = k_mat_compare_epsilon) const noexcept;

        /**
         * @brief Returns the negated matrix (element-wise negation)
         */
        mat3 operator-() const noexcept;

        /**
         * @brief Multiplies the matrix by a scalar
         */
        mat3 operator*(float a) const noexcept;

        /**
         * @brief Multiplies the matrix with a 3D vector
         */
        vec3 operator*(const vec3& v) const noexcept;

        /**
         * @brief Matrix multiplication
         */
        mat3 operator*(const mat3& m) const noexcept;

        /**
         * @brief Adds two matrices element-wise
         */
        mat3 operator+(const mat3& m) const noexcept;

        /**
         * @brief Subtracts two matrices element-wise
         */
        mat3 operator-(const mat3& m) const noexcept;

        /**
         * @brief In-place scalar multiplication
         */
        mat3& operator*=(const float a) noexcept;

        /**
         * @brief In-place matrix multiplication
         */
        mat3& operator*=(const mat3& m) noexcept;

        /**
         * @brief In-place element-wise addition
         */
        mat3& operator+=(const mat3& m) noexcept;

        /**
         * @brief In-place element-wise subtraction
         */
        mat3& operator-=(const mat3& m) noexcept;

        /**
         * @brief Transpose matrix
         */
        mat3 transpose() const noexcept;

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
         * (e.g., world-space to view-space). This method performs a full inverse of the 3x3 matrix.
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
        mat3 inverse() const noexcept;

        /**
         * @brief Returns the identity matrix
         */
        static constexpr mat3 identity() noexcept;

        /**
         * @brief Returns a pointer to the raw float array [col1, col2, col3]
         */
        const float* data() const noexcept;

        /**
         * @brief Returns a pointer to the raw float array [col1, col2, col3]
         */
        float* data() noexcept;

        /**
         * @brief Returns a string representation "[[c1.x, ..., c1.z], ..., [c3.x, ..., c3.z]]" with specified precision
         */
        string to_string(int precision = 3) const;

    public:
        vec3 cols[3];
    };

    static_assert(sizeof(mat3) == 36, "incorrect size");
    static_assert(alignof(mat3) == 4, "incorrect alignment");

    inline constexpr mat3::mat3(const vec3& col0, const vec3& col1, const vec3& col2) noexcept
        : cols{col0, col1, col2}
    {
    }

    inline constexpr mat3::mat3(float diag) noexcept
        : cols{
              vec3(diag, 0.0f, 0.0f),
              vec3(0.0f, diag, 0.0f),
              vec3(0.0f, 0.0f, diag),
          }
    {
    }

    inline constexpr mat3 mat3::identity() noexcept
    {
        return mat3(1.0f);
    }

} // namespace tavros::core::math

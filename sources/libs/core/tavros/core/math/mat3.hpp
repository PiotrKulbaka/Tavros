#pragma once

#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/quat.hpp>

namespace tavros::math
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
        /// @brief Returns the identity matrix
        static constexpr mat3 identity() noexcept;

        /// @brief Construct matrix from quaternion
        static mat3 from_quat(const quat& q) noexcept;

    public:
        /// @brief Default constructor, constructs a zero matrix
        constexpr mat3() noexcept;

        /// @brief Construct matrix from 9 components
        constexpr explicit mat3(
            float a00, float a01, float a02,
            float a10, float a11, float a12,
            float a20, float a21, float a22
        ) noexcept;

        /// @brief Construct from 3 columns
        constexpr explicit mat3(const vec3& col0, const vec3& col1, const vec3& col2) noexcept;

        /// @brief Construct from scalar (diagonal matrix)
        constexpr explicit mat3(float diag) noexcept;

        /// @brief Accesses a component by index [0..3]. Asserts in debug if index is out of bounds
        constexpr vec3& operator[](size_t i) noexcept;

        /// @brief Accesses a component by index [0..3]. Asserts in debug if index is out of bounds
        constexpr const vec3& operator[](size_t i) const noexcept;

        /// @brief Deleted comparison. Use `almost_equal` instead
        bool operator==(const mat3& m) const = delete;

        /// @brief Deleted comparison. Use `almost_equal` instead
        bool operator!=(const mat3& m) const = delete;

        /// @brief Adds another matrix to this one
        constexpr mat3& operator+=(const mat3& m) noexcept;

        /// @brief Subtracts another matrix from this one
        constexpr mat3& operator-=(const mat3& m) noexcept;

        /// @brief Multiplies this matrix by another
        constexpr mat3& operator*=(const mat3& m) noexcept;

        /// @brief Multiplies this matrix by a scalar
        constexpr mat3& operator*=(float s) noexcept;

        /// @brief Returns a pointer to the raw float array [col1, col2, col3, col4]
        constexpr const float* data() const noexcept;

        /// @brief Returns a pointer to the raw float array [col1, col2, col3, col4]
        constexpr float* data() noexcept;

    public:
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4458)

        union
        {
            struct
            {
                vec3 cols[3];
            };
            struct
            {
                vec3 col0, col1, col2;
            };
        };
    };

#pragma warning(pop)

    constexpr mat3 operator-(const mat3& m) noexcept;
    constexpr mat3 operator+(const mat3& a, const mat3& b) noexcept;
    constexpr mat3 operator-(const mat3& a, const mat3& b) noexcept;
    constexpr mat3 operator*(const mat3& a, const mat3& b) noexcept;
    constexpr vec3 operator*(const mat3& m, const vec3& v) noexcept;
    constexpr vec3 operator*(const vec3& v, const mat3& m) noexcept;
    constexpr mat3 operator*(const mat3& m, float s) noexcept;
    constexpr mat3 operator*(float s, const mat3& m) noexcept;
    constexpr mat3 operator/(const mat3& m, float s) noexcept;
    constexpr mat3 operator/(float s, const mat3& m) noexcept;

} // namespace tavros::math

#pragma warning(push)
#pragma warning(disable : 4458)
#include <tavros/core/math/mat3.inl>
#pragma warning(pop)


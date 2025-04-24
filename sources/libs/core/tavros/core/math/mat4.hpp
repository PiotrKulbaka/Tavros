#pragma once

#include <tavros/core/math/vec4.hpp>
#include <tavros/core/math/quat.hpp>

namespace tavros::math
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
        /// @brief Returns the identity matrix
        static constexpr mat4 identity() noexcept;

        /// @brief Construct matrix from quaternion
        static mat4 from_quat(const quat& q) noexcept;

    public:
        /// @brief Default constructor, constructs a zero matrix
        constexpr mat4() noexcept;

        /// @brief Construct matrix from 16 components
        constexpr explicit mat4(
            float a00, float a01, float a02, float a03,
            float a10, float a11, float a12, float a13,
            float a20, float a21, float a22, float a23,
            float a30, float a31, float a32, float a33
        ) noexcept;

        /// @brief Construct from 4 columns
        constexpr explicit mat4(const vec4& col0, const vec4& col1, const vec4& col2, const vec4& col3) noexcept;

        /// @brief Construct from scalar (diagonal matrix)
        constexpr explicit mat4(float diag) noexcept;

        /// @brief Accesses a component by index [0..3]. Asserts in debug if index is out of bounds
        constexpr vec4& operator[](size_t i) noexcept;

        /// @brief Accesses a component by index [0..3]. Asserts in debug if index is out of bounds
        constexpr const vec4& operator[](size_t i) const noexcept;

        /// @brief Deleted comparison. Use `almost_equal` instead
        bool operator==(const mat4& m) const = delete;

        /// @brief Deleted comparison. Use `almost_equal` instead
        bool operator!=(const mat4& m) const = delete;

        /// @brief Adds another matrix to this one
        constexpr mat4& operator+=(const mat4& m) noexcept;

        /// @brief Subtracts another matrix from this one
        constexpr mat4& operator-=(const mat4& m) noexcept;

        /// @brief Multiplies this matrix by another
        constexpr mat4& operator*=(const mat4& m) noexcept;

        /// @brief Multiplies this matrix by a scalar
        constexpr mat4& operator*=(float s) noexcept;

        /// @brief Returns a pointer to the raw float array [col1, col2, col3, col4]
        constexpr const float* data() const noexcept;

        /// @brief Returns a pointer to the raw float array [col1, col2, col3, col4]
        constexpr float* data() noexcept;

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


    constexpr mat4 operator-(const mat4& m) noexcept;
    constexpr mat4 operator+(const mat4& a, const mat4& b) noexcept;
    constexpr mat4 operator-(const mat4& a, const mat4& b) noexcept;
    constexpr mat4 operator*(const mat4& a, const mat4& b) noexcept;
    constexpr vec4 operator*(const mat4& m, const vec4& v) noexcept;
    constexpr vec4 operator*(const vec4& v, const mat4& m) noexcept;
    constexpr mat4 operator*(const mat4& m, float s) noexcept;
    constexpr mat4 operator*(float s, const mat4& m) noexcept;
    constexpr mat4 operator/(const mat4& m, float s) noexcept;
    constexpr mat4 operator/(float s, const mat4& m) noexcept;

} // namespace tavros::math

#include <tavros/core/math/mat4.inl>

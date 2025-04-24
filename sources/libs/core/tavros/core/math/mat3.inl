#include <tavros/core/math/mat3.hpp>

#include <tavros/core/math/functions/make_mat.hpp>

namespace tavros::math
{

    mat3 make_mat3(const quat& q) noexcept;

    static_assert(sizeof(mat3) == 36, "incorrect size");
    static_assert(alignof(mat3) == 4, "incorrect alignment");

    inline constexpr mat3 mat3::identity() noexcept
    {
        return mat3(1.0f);
    }

    inline mat3 mat3::from_quat(const quat& q) noexcept
    {
        return make_mat3(q);
    }

    inline constexpr mat3::mat3() noexcept
        : mat3(0.0f)
    {
    }

    inline constexpr mat3::mat3(const vec3& col0, const vec3& col1, const vec3& col2) noexcept
        : cols{col0, col1, col2}
    {
    }

    inline constexpr mat3::mat3(
        float a00, float a01, float a02,
        float a10, float a11, float a12,
        float a20, float a21, float a22
    ) noexcept
        : col0(a00, a01, a02)
        , col1(a10, a11, a12)
        , col2(a20, a21, a22)
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

    inline constexpr vec3& mat3::operator[](size_t i) noexcept
    {
        TAV_ASSERT(i < 3);
        return cols[i];
    }

    inline constexpr const vec3& mat3::operator[](size_t i) const noexcept
    {
        TAV_ASSERT(i < 3);
        return cols[i];
    }

    inline constexpr mat3& mat3::operator+=(const mat3& m) noexcept
    {
        cols[0] += m[0];
        cols[1] += m[1];
        cols[2] += m[2];
        return *this;
    }

    inline constexpr mat3& mat3::operator-=(const mat3& m) noexcept
    {
        cols[0] -= m[0];
        cols[1] -= m[1];
        cols[2] -= m[2];
        return *this;
    }

    inline constexpr mat3& mat3::operator*=(const mat3& m) noexcept
    {
        *this = *this * m;
        return *this;
    }

    inline constexpr mat3& mat3::operator*=(float s) noexcept
    {
        cols[0] *= s;
        cols[1] *= s;
        cols[2] *= s;
        return *this;
    }

    inline constexpr const float* mat3::data() const noexcept
    {
        return cols[0].data();
    }

    inline constexpr float* mat3::data() noexcept
    {
        return cols[0].data();
    }

    inline constexpr mat3 operator-(const mat3& m) noexcept
    {
        return mat3(-m[0], -m[1], -m[2]);
    }

    inline constexpr mat3 operator+(const mat3& a, const mat3& b) noexcept
    {
        return mat3(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
    }

    inline constexpr mat3 operator-(const mat3& a, const mat3& b) noexcept
    {
        return mat3(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
    }

    inline constexpr mat3 operator*(const mat3& a, const mat3& b) noexcept
    {
        mat3 result;
        for (size_t col = 0; col < 3; ++col) {
            for (size_t row = 0; row < 3; ++row) {
                // Compute the element at [row][col] as a dot product of:
                // - the 'row'-th row of this matrix
                // - the 'col'-th column of matrix 'm'
                // Note: in column-major layout, rows are accessed as [col][row]
                result[col][row] =
                    a[0][row] * b[col][0] + a[1][row] * b[col][1] + a[2][row] * b[col][2];
            }
        }
        return result;
    }

    inline constexpr vec3 operator*(const mat3& m, const vec3& v) noexcept
    {
        return m[0] * v.x + m[1] * v.y + m[2] * v.z;
    }

    inline constexpr vec3 operator*(const vec3& v, const mat3& m) noexcept
    {
        return vec3(
            v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0],
            v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1],
            v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2]
        );
    }

    inline constexpr mat3 operator*(const mat3& m, float s) noexcept
    {
        return mat3(m[0] * s, m[1] * s, m[2] * s);
    }

    inline constexpr mat3 operator*(float s, const mat3& m) noexcept
    {
        return mat3(m[0] * s, m[1] * s, m[2] * s);
    }

    inline constexpr mat3 operator/(const mat3& m, float s) noexcept
    {
        return mat3(m[0] / s, m[1] / s, m[2] / s);
    }

    inline constexpr mat3 operator/(float s, const mat3& m) noexcept
    {
        return mat3(s / m[0], s / m[1], s / m[2]);
    }

} // namespace tavros::math

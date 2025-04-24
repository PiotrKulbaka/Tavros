#include <tavros/core/math/mat4.hpp>

#include <tavros/core/math/functions/make_mat.hpp>

namespace tavros::math
{

    mat3 make_mat3(const quat& q) noexcept;
    mat4 make_mat4(const quat& q) noexcept;

    static_assert(sizeof(mat4) == 64, "incorrect size");
    static_assert(alignof(mat4) == 16, "incorrect alignment");

    inline constexpr mat4 mat4::identity() noexcept
    {
        return mat4(1.0f);
    }

    inline mat4 mat4::from_quat(const quat& q) noexcept
    {
        return make_mat4(q);
    }

    inline constexpr mat4::mat4() noexcept
        : mat4(0.0f)
    {
    }

    inline constexpr mat4::mat4(
        float a00, float a01, float a02, float a03,
        float a10, float a11, float a12, float a13,
        float a20, float a21, float a22, float a23,
        float a30, float a31, float a32, float a33
    ) noexcept
        : col0(a00, a01, a02, a03)
        , col1(a10, a11, a12, a13)
        , col2(a20, a21, a22, a23)
        , col3(a30, a31, a32, a33)
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

    inline constexpr vec4& mat4::operator[](size_t i) noexcept
    {
        TAV_ASSERT(i < 4);
        return cols[i];
    }

    inline constexpr const vec4& mat4::operator[](size_t i) const noexcept
    {
        TAV_ASSERT(i < 4);
        return cols[i];
    }

    inline constexpr mat4& mat4::operator*=(const mat4& m) noexcept
    {
        *this = *this * m;
        return *this;
    }

    inline constexpr mat4& mat4::operator+=(const mat4& m) noexcept
    {
        cols[0] += m[0];
        cols[1] += m[1];
        cols[2] += m[2];
        cols[3] += m[3];
        return *this;
    }

    inline constexpr mat4& mat4::operator-=(const mat4& m) noexcept
    {
        cols[0] -= m[0];
        cols[1] -= m[1];
        cols[2] -= m[2];
        cols[3] -= m[3];
        return *this;
    }

    inline constexpr mat4& mat4::operator*=(float s) noexcept
    {
        cols[0] *= s;
        cols[1] *= s;
        cols[2] *= s;
        cols[3] *= s;
        return *this;
    }

    inline constexpr const float* mat4::data() const noexcept
    {
        return cols[0].data();
    }

    inline constexpr float* mat4::data() noexcept
    {
        return cols[0].data();
    }

    inline constexpr mat4 operator-(const mat4& m) noexcept
    {
        return mat4(-m[0], -m[1], -m[2], -m[3]);
    }

    inline constexpr mat4 operator+(const mat4& a, const mat4& b) noexcept
    {
        return mat4(a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]);
    }

    inline constexpr mat4 operator-(const mat4& a, const mat4& b) noexcept
    {
        return mat4(a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]);
    }

    inline constexpr mat4 operator*(const mat4& a, const mat4& b) noexcept
    {
        mat4 result;
        for (size_t col = 0; col < 4; ++col) {
            for (size_t row = 0; row < 4; ++row) {
                // Compute the element at [row][col] as a dot product of:
                // - the 'row'-th row of this matrix
                // - the 'col'-th column of matrix 'm'
                // Note: in column-major layout, rows are accessed as [col][row]
                result[col][row] =
                    a[0][row] * b[col][0] + a[1][row] * b[col][1] + a[2][row] * b[col][2] + a[3][row] * b[col][3];
            }
        }
        return result;
    }

    inline constexpr vec4 operator*(const mat4& m, const vec4& v) noexcept
    {
        return m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w;
    }

    inline constexpr vec4 operator*(const vec4& v, const mat4& m) noexcept
    {
        return vec4(
            v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + v.w * m[3][0],
            v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + v.w * m[3][1],
            v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + v.w * m[3][2],
            v.x * m[0][3] + v.y * m[1][3] + v.z * m[2][3] + v.w * m[3][3]
        );
    }

    inline constexpr mat4 operator*(const mat4& m, float s) noexcept
    {
        return mat4(m[0] * s, m[1] * s, m[2] * s, m[3] * s);
    }

    inline constexpr mat4 operator*(float s, const mat4& m) noexcept
    {
        return mat4(m[0] * s, m[1] * s, m[2] * s, m[3] * s);
    }

    inline constexpr mat4 operator/(const mat4& m, float s) noexcept
    {
        return mat4(m[0] / s, m[1] / s, m[2] / s, m[3] / s);
    }

    inline constexpr mat4 operator/(float s, const mat4& m) noexcept
    {
        return mat4(s / m[0], s / m[1], s / m[2], s / m[3]);
    }

} // namespace tavros::math

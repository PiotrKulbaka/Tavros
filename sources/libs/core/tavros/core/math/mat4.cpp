#include <tavros/core/math/mat4.hpp>

#include <tavros/core/debug/assert.hpp>

#include <cmath>

using namespace tavros::core::math;

vec4& mat4::operator[](size_t i) noexcept
{
    TAV_ASSERT(i < 4);
    return cols[i];
}

const vec4& mat4::operator[](size_t i) const noexcept
{
    TAV_ASSERT(i < 4);
    return cols[i];
}

bool mat4::operator==(const mat4& m) const noexcept
{
    return cols[0] == m[0] && cols[1] == m[1] && cols[2] == m[2] && cols[3] == m[3];
}

bool mat4::operator!=(const mat4& m) const noexcept
{
    return !(*this == m);
}

bool mat4::almost_equal(const mat4& m, float epsilon) const noexcept
{
    const float* a = ptr();
    const float* b = m.ptr();
    for (auto i = 0; i < 16; ++i) {
        if (std::abs(a[i] - b[i]) > epsilon) {
            return false;
        }
    }
    return true;
}

mat4 mat4::operator-() const noexcept
{
    return mat4(-cols[0], -cols[1], -cols[2], -cols[3]);
}

mat4 mat4::operator*(float a) const noexcept
{
    return mat4(cols[0] * a, cols[1] * a, cols[2] * a, cols[3] * a);
}

vec4 mat4::operator*(const vec4& v) const noexcept
{
    // Column-major matrix * vector
    return cols[0] * v.x + cols[1] * v.y + cols[2] * v.z + cols[3] * v.w;
}

mat4 mat4::operator*(const mat4& m) const noexcept
{
    mat4 result;
    for (size_t col = 0; col < 4; ++col) {
        for (size_t row = 0; row < 4; ++row) {
            // clang-format off
            // Compute the element at [row][col] as a dot product of:
            // - the 'row'-th row of this matrix
            // - the 'col'-th column of matrix 'm'
            // Note: in column-major layout, rows are accessed as [col][row]
            result[col][row] =
                cols[0][row] * m[col][0] +
                cols[1][row] * m[col][1] +
                cols[2][row] * m[col][2] +
                cols[3][row] * m[col][3];
            // clang-format on
        }
    }
    return result;
}

mat4 mat4::operator+(const mat4& m) const noexcept
{
    return mat4(cols[0] + m[0], cols[1] + m[1], cols[2] + m[2], cols[3] + m[3]);
}

mat4 mat4::operator-(const mat4& m) const noexcept
{
    return mat4(cols[0] - m[0], cols[1] - m[1], cols[2] - m[2], cols[3] - m[3]);
}

mat4& mat4::operator*=(const float a)
{
    cols[0] *= a;
    cols[1] *= a;
    cols[2] *= a;
    cols[3] *= a;
    return *this;
}

mat4& mat4::operator*=(const mat4& m)
{
    *this = *this * m;
    return *this;
}

mat4& mat4::operator+=(const mat4& m)
{
    cols[0] += m[0];
    cols[1] += m[1];
    cols[2] += m[2];
    cols[3] += m[3];
    return *this;
}

mat4& mat4::operator-=(const mat4& m)
{
    cols[0] -= m[0];
    cols[1] -= m[1];
    cols[2] -= m[2];
    cols[3] -= m[3];
    return *this;
}

mat4 mat4::transpose() const noexcept
{
    return mat4(
        vec4(cols[0].x, cols[1].x, cols[2].x, cols[3].x),
        vec4(cols[0].y, cols[1].y, cols[2].y, cols[3].y),
        vec4(cols[0].z, cols[1].z, cols[2].z, cols[3].z),
        vec4(cols[0].w, cols[1].w, cols[2].w, cols[3].w)
    );
}

float mat4::determinant() const noexcept
{
    // clang-format off
    const float a0 = cols[0].x * (
          cols[1].y * (cols[2].z * cols[3].w - cols[3].z * cols[2].w)
        - cols[2].y * (cols[1].z * cols[3].w - cols[3].z * cols[1].w)
        + cols[3].y * (cols[1].z * cols[2].w - cols[2].z * cols[1].w));

    const float a1 = cols[1].x * (
          cols[0].y * (cols[2].z * cols[3].w - cols[3].z * cols[2].w)
        - cols[2].y * (cols[0].z * cols[3].w - cols[3].z * cols[0].w)
        + cols[3].y * (cols[0].z * cols[2].w - cols[2].z * cols[0].w));

    const float a2 = cols[2].x * (
          cols[0].y * (cols[1].z * cols[3].w - cols[3].z * cols[1].w)
        - cols[1].y * (cols[0].z * cols[3].w - cols[3].z * cols[0].w)
        + cols[3].y * (cols[0].z * cols[1].w - cols[1].z * cols[0].w));

    const float a3 = cols[3].x * (
          cols[0].y * (cols[1].z * cols[2].w - cols[2].z * cols[1].w)
        - cols[1].y * (cols[0].z * cols[2].w - cols[2].z * cols[0].w)
        + cols[2].y * (cols[0].z * cols[1].w - cols[1].z * cols[0].w));
    // clang-format on
    return a0 - a1 + a2 - a3;
}

mat4 mat4::inverse() const noexcept
{
    // For future, links for quick calculation of the inverse matrix:
    // https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html?utm_source
    // https://github.com/g-truc/glm/blob/master/glm/gtc/matrix_inverse.hpp
    mat4 inv_mat;

    const float* m = ptr();
    float*       inv = inv_mat.ptr();

    // Calculation of minors
    // clang-format off
    inv[0] =
          m[5] * m[10] * m[15]
        - m[5] * m[11] * m[14]
        - m[9] * m[6]  * m[15]
        + m[9] * m[7]  * m[14]
        + m[13]* m[6]  * m[11]
        - m[13]* m[7]  * m[10];

    inv[1] =  
        - m[1] * m[10] * m[15]
        + m[1] * m[11] * m[14]
        + m[9] * m[2]  * m[15]
        - m[9] * m[3]  * m[14]
        - m[13]* m[2]  * m[11]
        + m[13]* m[3]  * m[10];

    inv[2] = 
          m[1] * m[6]  * m[15]
        - m[1] * m[7]  * m[14]
        - m[5] * m[2]  * m[15]
        + m[5] * m[3]  * m[14]
        + m[13]* m[2]  * m[7]
        - m[13]* m[3]  * m[6];

    inv[3] =
        - m[1] * m[6]  * m[11]
        + m[1] * m[7]  * m[10]
        + m[5] * m[2]  * m[11]
        - m[5] * m[3]  * m[10]
        - m[9] * m[2]  * m[7]
        + m[9] * m[3]  * m[6];

    inv[4] =
        - m[4] * m[10] * m[15]
        + m[4] * m[11] * m[14]
        + m[8] * m[6]  * m[15]
        - m[8] * m[7]  * m[14]
        - m[12]* m[6]  * m[11]
        + m[12]* m[7]  * m[10];

    inv[5] =
          m[0] * m[10] * m[15]
        - m[0] * m[11] * m[14]
        - m[8] * m[2]  * m[15]
        + m[8] * m[3]  * m[14]
        + m[12]* m[2]  * m[11]
        - m[12]* m[3]  * m[10];

    inv[6] = 
        - m[0] * m[6]  * m[15]
        + m[0] * m[7]  * m[14]
        + m[4] * m[2]  * m[15]
        - m[4] * m[3]  * m[14]
        - m[12]* m[2]  * m[7]
        + m[12]* m[3]  * m[6];

    inv[7] =
          m[0] * m[6]  * m[11]
        - m[0] * m[7]  * m[10]
        - m[4] * m[2]  * m[11]
        + m[4] * m[3]  * m[10]
        + m[8] * m[2]  * m[7]
        - m[8] * m[3]  * m[6];

    inv[8] =
        m[4] * m[9]  * m[15]
        - m[4] * m[11] * m[13]
        - m[8] * m[5]  * m[15]
        + m[8] * m[7]  * m[13]
        + m[12]* m[5]  * m[11]
        - m[12]* m[7]  * m[9];

    inv[9] =
        - m[0] * m[9]  * m[15]
        + m[0] * m[11] * m[13]
        + m[8] * m[1]  * m[15]
        - m[8] * m[3]  * m[13]
        - m[12]* m[1]  * m[11]
        + m[12]* m[3]  * m[9];

    inv[10] =
          m[0] * m[5]  * m[15]
        - m[0] * m[7]  * m[13]
        - m[4] * m[1]  * m[15]
        + m[4] * m[3]  * m[13]
        + m[12]* m[1]  * m[7]
        - m[12]* m[3]  * m[5];

    inv[11] = 
        - m[0] * m[5]  * m[11]
        + m[0] * m[7]  * m[9]
        + m[4] * m[1]  * m[11]
        - m[4] * m[3]  * m[9]
        - m[8] * m[1]  * m[7]
        + m[8] * m[3]  * m[5];

    inv[12] = 
        - m[4] * m[9]  * m[14]
        + m[4] * m[10] * m[13]
        + m[8] * m[5]  * m[14]
        - m[8] * m[6]  * m[13]
        - m[12]* m[5]  * m[10]
        + m[12]* m[6]  * m[9];

    inv[13] =
          m[0] * m[9]  * m[14]
        - m[0] * m[10] * m[13]
        - m[8] * m[1]  * m[14]
        + m[8] * m[2]  * m[13]
        + m[12]* m[1]  * m[10]
        - m[12]* m[2]  * m[9];

    inv[14] = 
        - m[0] * m[5]  * m[14]
        + m[0] * m[6]  * m[13]
        + m[4] * m[1]  * m[14]
        - m[4] * m[2]  * m[13]
        - m[12]* m[1]  * m[6]
        + m[12]* m[2]  * m[5];

    inv[15] =
          m[0] * m[5]  * m[10]
        - m[0] * m[6]  * m[9]
        - m[4] * m[1]  * m[10]
        + m[4] * m[2]  * m[9]
        + m[8] * m[1]  * m[6]
        - m[8] * m[2]  * m[5];
    // clang-format on

    // Calc det by the first row
    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    TAV_ASSERT(det != 0.0f);
    if (det == 0.0f) {
        // Can't calculate inverse matrix, so return matrix with uninitialized values
        return mat4();
    }

    // Apply inverse determinant to the whole matrix
    inv_mat *= 1.0f / det;

    return inv_mat;
}

const float* mat4::ptr() const noexcept
{
    return cols[0].ptr();
}

float* mat4::ptr() noexcept
{
    return cols[0].ptr();
}

tavros::core::string mat4::to_string(int precision) const
{
    char buffer[256];
    // clang-format off
    snprintf(buffer, sizeof(buffer), 
        "[[%.*f, %.*f, %.*f, %.*f], [%.*f, %.*f, %.*f, %.*f], [%.*f, %.*f, %.*f, %.*f], [%.*f, %.*f, %.*f, %.*f]]",
        precision, cols[0].x, precision, cols[0].y, precision, cols[0].z, precision, cols[0].w,
        precision, cols[1].x, precision, cols[1].y, precision, cols[1].z, precision, cols[1].w,
        precision, cols[2].x, precision, cols[2].y, precision, cols[2].z, precision, cols[2].w,
        precision, cols[3].x, precision, cols[3].y, precision, cols[3].z, precision, cols[3].w
    );
    // clang-format on
    return string(buffer);
}

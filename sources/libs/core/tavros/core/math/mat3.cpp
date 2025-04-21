#include <tavros/core/math/mat3.hpp>

#include <tavros/core/debug/assert.hpp>

using namespace tavros::core::math;

vec3& mat3::operator[](size_t i) noexcept
{
    TAV_ASSERT(i < 3);
    return cols[i];
}

const vec3& mat3::operator[](size_t i) const noexcept
{
    TAV_ASSERT(i < 3);
    return cols[i];
}

bool mat3::almost_equal(const mat3& m, float epsilon) const noexcept
{
    const float* a = data();
    const float* b = m.data();
    for (auto i = 0; i < 9; ++i) {
        if (std::abs(a[i] - b[i]) > epsilon) {
            return false;
        }
    }
    return true;
}

mat3 mat3::operator-() const noexcept
{
    return mat3(-cols[0], -cols[1], -cols[2]);
}

mat3 mat3::operator*(float a) const noexcept
{
    return mat3(cols[0] * a, cols[1] * a, cols[2] * a);
}

vec3 mat3::operator*(const vec3& v) const noexcept
{
    // Column-major matrix * vector
    return cols[0] * v.x + cols[1] * v.y + cols[2] * v.z;
}

mat3 mat3::operator*(const mat3& m) const noexcept
{
    mat3 result;
    for (size_t col = 0; col < 3; ++col) {
        for (size_t row = 0; row < 3; ++row) {
            // clang-format off
            // Compute the element at [row][col] as a dot product of:
            // - the 'row'-th row of this matrix
            // - the 'col'-th column of matrix 'm'
            // Note: in column-major layout, rows are accessed as [col][row]
            result[col][row] =
                cols[0][row] * m[col][0] +
                cols[1][row] * m[col][1] +
                cols[2][row] * m[col][2];
            // clang-format on
        }
    }
    return result;
}

mat3 mat3::operator+(const mat3& m) const noexcept
{
    return mat3(cols[0] + m[0], cols[1] + m[1], cols[2] + m[2]);
}

mat3 mat3::operator-(const mat3& m) const noexcept
{
    return mat3(cols[0] - m[0], cols[1] - m[1], cols[2] - m[2]);
}

mat3& mat3::operator*=(const float a) noexcept
{
    cols[0] *= a;
    cols[1] *= a;
    cols[2] *= a;
    return *this;
}

mat3& mat3::operator*=(const mat3& m) noexcept
{
    *this = *this * m;
    return *this;
}

mat3& mat3::operator+=(const mat3& m) noexcept
{
    cols[0] += m[0];
    cols[1] += m[1];
    cols[2] += m[2];
    return *this;
}

mat3& mat3::operator-=(const mat3& m) noexcept
{
    cols[0] -= m[0];
    cols[1] -= m[1];
    cols[2] -= m[2];
    return *this;
}

mat3 mat3::transpose() const noexcept
{
    return mat3(
        vec3(cols[0].x, cols[1].x, cols[2].x),
        vec3(cols[0].y, cols[1].y, cols[2].y),
        vec3(cols[0].z, cols[1].z, cols[2].z)
    );
}

float mat3::determinant() const noexcept
{
    // clang-format off
    return
          cols[0].x * (cols[1].y * cols[2].z - cols[1].z * cols[2].y)
        - cols[1].x * (cols[0].y * cols[2].z - cols[0].z * cols[2].y)
        + cols[2].x * (cols[0].y * cols[1].z - cols[0].z * cols[1].y);
    // clang-format on
}

mat3 mat3::inverse() const noexcept
{
    // For future, links for quick calculation of the inverse matrix:
    // https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html?utm_source
    // https://github.com/g-truc/glm/blob/master/glm/gtc/matrix_inverse.hpp
    mat3 inv_mat;

    const float* m = data();
    float*       inv = inv_mat.data();

    // clang-format off
    inv[0] =  m[4] * m[8] - m[5] * m[7]; // Cofactor00
    inv[3] = -m[3] * m[8] + m[5] * m[6]; // Cofactor10
    inv[6] =  m[3] * m[7] - m[4] * m[6]; // Cofactor20

    inv[1] = -m[1] * m[8] + m[2] * m[7]; // Cofactor01
    inv[4] =  m[0] * m[8] - m[2] * m[6]; // Cofactor11
    inv[7] = -m[0] * m[7] + m[1] * m[6]; // Cofactor21

    inv[2] =  m[1] * m[5] - m[2] * m[4]; // Cofactor02
    inv[5] = -m[0] * m[5] + m[2] * m[3]; // Cofactor12
    inv[8] =  m[0] * m[4] - m[1] * m[3]; // Cofactor22
    // clang-format on

    float det = m[0] * inv[0] + m[1] * inv[3] + m[2] * inv[6];
    TAV_ASSERT(!almost_zero(det, k_epsilon6));
    if (almost_zero(det, k_epsilon6)) {
        // Can't calculate inverse matrix, so return zero matrix
        return mat3(0.0f);
    }

    inv_mat *= 1.0f / det;
    return inv_mat;
}

const float* mat3::data() const noexcept
{
    return cols[0].data();
}

float* mat3::data() noexcept
{
    return cols[0].data();
}

tavros::core::string mat3::to_string(int precision) const
{
    char buffer[256];
    // clang-format off
    snprintf(buffer, sizeof(buffer), 
        "[[%.*f, %.*f, %.*f], [%.*f, %.*f, %.*f], [%.*f, %.*f, %.*f]]",
        precision, cols[0].x, precision, cols[0].y, precision, cols[0].z,
        precision, cols[1].x, precision, cols[1].y, precision, cols[1].z,
        precision, cols[2].x, precision, cols[2].y, precision, cols[2].z
    );
    // clang-format on
    return string(buffer);
}

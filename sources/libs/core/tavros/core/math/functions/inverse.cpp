#include <tavros/core/math/functions/inverse.hpp>

namespace tavros::math
{

    mat3 inverse(const mat3& m) noexcept
    {
        // https://github.com/g-truc/glm/blob/master/glm/gtc/matrix_inverse.inl#L43
        // clang-format off
        auto inv = mat3(
            + m[1][1] * m[2][2] - m[1][2] * m[2][1],
            - m[0][1] * m[2][2] + m[0][2] * m[2][1],
            + m[0][1] * m[1][2] - m[0][2] * m[1][1],
            - m[1][0] * m[2][2] + m[1][2] * m[2][0],
            + m[0][0] * m[2][2] - m[0][2] * m[2][0], 
            - m[0][0] * m[1][2] + m[0][2] * m[1][0],
            + m[1][0] * m[2][1] - m[1][1] * m[2][0],
            - m[0][0] * m[2][1] + m[0][1] * m[2][0],
            + m[0][0] * m[1][1] - m[0][1] * m[1][0]
        );
        // clang-format on

        const auto det = inv[0][0] * m[0][0] + inv[1][0] * m[0][1] + inv[2][0] * m[0][2];
        TAV_ASSERT(!almost_zero(det, k_epsilon6));
        if (almost_zero(det, k_epsilon6)) {
            // Can't calculate inverse matrix, so return zero matrix
            return mat3(0.0f);
        }

        inv *= 1.0f / det;
        return inv;
    }

    mat4 inverse(const mat4& m) noexcept
    {
        // For future, links for quick calculation of the inverse matrix:
        // https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html?utm_source
        // https://github.com/g-truc/glm/blob/master/glm/gtc/matrix_inverse.inl#L66
        mat4 inv;

        // clang-format off
        inv[0][0] =
            + m[1][1] * m[2][2] * m[3][3]
            - m[1][1] * m[2][3] * m[3][2]
            - m[2][1] * m[1][2] * m[3][3]
            + m[2][1] * m[1][3] * m[3][2]
            + m[3][1] * m[1][2] * m[2][3]
            - m[3][1] * m[1][3] * m[2][2];

        inv[0][1] =  
            - m[0][1] * m[2][2] * m[3][3]
            + m[0][1] * m[2][3] * m[3][2]
            + m[2][1] * m[0][2] * m[3][3]
            - m[2][1] * m[0][3] * m[3][2]
            - m[3][1] * m[0][2] * m[2][3]
            + m[3][1] * m[0][3] * m[2][2];

        inv[0][2] = 
            + m[0][1] * m[1][2] * m[3][3]
            - m[0][1] * m[1][3] * m[3][2]
            - m[1][1] * m[0][2] * m[3][3]
            + m[1][1] * m[0][3] * m[3][2]
            + m[3][1] * m[0][2] * m[1][3]
            - m[3][1] * m[0][3] * m[1][2];

        inv[0][3] =
            - m[0][1] * m[1][2] * m[2][3]
            + m[0][1] * m[1][3] * m[2][2]
            + m[1][1] * m[0][2] * m[2][3]
            - m[1][1] * m[0][3] * m[2][2]
            - m[2][1] * m[0][2] * m[1][3]
            + m[2][1] * m[0][3] * m[1][2];

        inv[1][0] =
            - m[1][0] * m[2][2] * m[3][3]
            + m[1][0] * m[2][3] * m[3][2]
            + m[2][0] * m[1][2] * m[3][3]
            - m[2][0] * m[1][3] * m[3][2]
            - m[3][0] * m[1][2] * m[2][3]
            + m[3][0] * m[1][3] * m[2][2];

        inv[1][1] =
            + m[0][0] * m[2][2] * m[3][3]
            - m[0][0] * m[2][3] * m[3][2]
            - m[2][0] * m[0][2] * m[3][3]
            + m[2][0] * m[0][3] * m[3][2]
            + m[3][0] * m[0][2] * m[2][3]
            - m[3][0] * m[0][3] * m[2][2];

        inv[1][2] = 
            - m[0][0] * m[1][2] * m[3][3]
            + m[0][0] * m[1][3] * m[3][2]
            + m[1][0] * m[0][2] * m[3][3]
            - m[1][0] * m[0][3] * m[3][2]
            - m[3][0] * m[0][2] * m[1][3]
            + m[3][0] * m[0][3] * m[1][2];

        inv[1][3] =
            + m[0][0] * m[1][2] * m[2][3]
            - m[0][0] * m[1][3] * m[2][2]
            - m[1][0] * m[0][2] * m[2][3]
            + m[1][0] * m[0][3] * m[2][2]
            + m[2][0] * m[0][2] * m[1][3]
            - m[2][0] * m[0][3] * m[1][2];

        inv[2][0] =
            + m[1][0] * m[2][1] * m[3][3]
            - m[1][0] * m[2][3] * m[3][1]
            - m[2][0] * m[1][1] * m[3][3]
            + m[2][0] * m[1][3] * m[3][1]
            + m[3][0] * m[1][1] * m[2][3]
            - m[3][0] * m[1][3] * m[2][1];

        inv[2][1] =
            - m[0][0] * m[2][1] * m[3][3]
            + m[0][0] * m[2][3] * m[3][1]
            + m[2][0] * m[0][1] * m[3][3]
            - m[2][0] * m[0][3] * m[3][1]
            - m[3][0] * m[0][1] * m[2][3]
            + m[3][0] * m[0][3] * m[2][1];

        inv[2][2] =
            + m[0][0] * m[1][1] * m[3][3]
            - m[0][0] * m[1][3] * m[3][1]
            - m[1][0] * m[0][1] * m[3][3]
            + m[1][0] * m[0][3] * m[3][1]
            + m[3][0] * m[0][1] * m[1][3]
            - m[3][0] * m[0][3] * m[1][1];

        inv[2][3] = 
            - m[0][0] * m[1][1] * m[2][3]
            + m[0][0] * m[1][3] * m[2][1]
            + m[1][0] * m[0][1] * m[2][3]
            - m[1][0] * m[0][3] * m[2][1]
            - m[2][0] * m[0][1] * m[1][3]
            + m[2][0] * m[0][3] * m[1][1];

        inv[3][0] = 
            - m[1][0] * m[2][1] * m[3][2]
            + m[1][0] * m[2][2] * m[3][1]
            + m[2][0] * m[1][1] * m[3][2]
            - m[2][0] * m[1][2] * m[3][1]
            - m[3][0] * m[1][1] * m[2][2]
            + m[3][0] * m[1][2] * m[2][1];

        inv[3][1] =
            + m[0][0] * m[2][1] * m[3][2]
            - m[0][0] * m[2][2] * m[3][1]
            - m[2][0] * m[0][1] * m[3][2]
            + m[2][0] * m[0][2] * m[3][1]
            + m[3][0] * m[0][1] * m[2][2]
            - m[3][0] * m[0][2] * m[2][1];

        inv[3][2] = 
            - m[0][0] * m[1][1] * m[3][2]
            + m[0][0] * m[1][2] * m[3][1]
            + m[1][0] * m[0][1] * m[3][2]
            - m[1][0] * m[0][2] * m[3][1]
            - m[3][0] * m[0][1] * m[1][2]
            + m[3][0] * m[0][2] * m[1][1];

        inv[3][3] =
            + m[0][0] * m[1][1] * m[2][2]
            - m[0][0] * m[1][2] * m[2][1]
            - m[1][0] * m[0][1] * m[2][2]
            + m[1][0] * m[0][2] * m[2][1]
            + m[2][0] * m[0][1] * m[1][2]
            - m[2][0] * m[0][2] * m[1][1];
        // clang-format on

        // Calc det by the first row
        float det = m[0][0] * inv[0][0] + m[0][1] * inv[1][0] + m[0][2] * inv[2][0] + m[0][3] * inv[3][0];
        TAV_ASSERT(!almost_zero(det, k_epsilon6));
        if (almost_zero(det, k_epsilon6)) {
            // Can't calculate inverse matrix, so return zero matrix
            return mat4(0.0f);
        }

        // Apply inverse determinant to the whole matrix
        inv *= 1.0f / det;
        return inv;
    }

    quat inverse(const quat& q) noexcept
    {
        return normalize(quat(-q.x, -q.y, -q.z, q.w));
    }

} // namespace tavros::math

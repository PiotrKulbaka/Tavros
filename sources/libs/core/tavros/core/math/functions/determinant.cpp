#include <tavros/core/math/functions/determinant.hpp>

namespace tavros::math
{

    float determinant(const mat3& m) noexcept
    {
        const auto a0 = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
        const auto a1 = m[1][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1]);
        const auto a2 = m[2][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);

        return a0 - a1 + a2;
    }

    float determinant(const mat4& m) noexcept
    {
        const auto a0 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        const auto a1 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
        const auto a2 = m[1][2] * m[2][3] - m[2][2] * m[1][3];
        const auto a3 = m[0][2] * m[3][3] - m[3][2] * m[0][3];
        const auto a4 = m[0][2] * m[2][3] - m[2][2] * m[0][3];
        const auto a5 = m[0][2] * m[1][3] - m[1][2] * m[0][3];

        const auto b0 = m[0][0] * (m[1][1] * a0 - m[2][1] * a1 + m[3][1] * a2);
        const auto b1 = m[1][0] * (m[0][1] * a0 - m[2][1] * a3 + m[3][1] * a4);
        const auto b2 = m[2][0] * (m[0][1] * a1 - m[1][1] * a3 + m[3][1] * a5);
        const auto b3 = m[3][0] * (m[0][1] * a2 - m[1][1] * a4 + m[2][1] * a5);

        return b0 - b1 + b2 - b3;
    }

} // namespace tavros::math

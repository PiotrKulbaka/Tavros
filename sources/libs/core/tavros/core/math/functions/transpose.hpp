#pragma once

/**
 * @file transpose.hpp
 * @brief Provides functions to compute the transpose of matrices
 *
 * The transpose of a matrix is obtained by swapping its rows with columns
 *
 * Supported types:
 * - `mat3`
 * - `mat4`
 */

#include <tavros/core/math/mat3.hpp>
#include <tavros/core/math/mat4.hpp>

namespace tavros::math
{

    inline constexpr mat3 transpose(const mat3& m) noexcept
    {
        return mat3(
            m[0].x, m[1].x, m[2].x,
            m[0].y, m[1].y, m[2].y,
            m[0].z, m[1].z, m[2].z
        );
    }

    inline constexpr mat4 transpose(const mat4& m) noexcept
    {
        return mat4(
            m[0].x, m[1].x, m[2].x, m[3].x,
            m[0].y, m[1].y, m[2].y, m[3].y,
            m[0].z, m[1].z, m[2].z, m[3].z,
            m[0].w, m[1].w, m[2].w, m[3].w
        );
    }

} // namespace tavros::math

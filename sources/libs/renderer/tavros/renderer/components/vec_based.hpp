#pragma once

#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>

namespace tavros::renderer
{
    /** 2D position */
    struct position2d_c
    {
        math::vec2 value;
    };

    /** Primary UV texture coordinates (channel 0) */
    struct uv0_c
    {
        math::vec2 value;
    };

    /** UV coordinates (channel 1) */
    struct uv1_c
    {
        math::vec2 value;
    };

    /** UV coordinates (channel 2) */
    struct uv2_c
    {
        math::vec2 value;
    };

    /** UV coordinates (channel 3) */
    struct uv3_c
    {
        math::vec2 value;
    };

    /** 3D position */
    struct position_c
    {
        math::vec3 value;
    };

    /** 3D normal */
    struct normal_c
    {
        math::vec3 value;
    };

} // namespace tavros::renderer

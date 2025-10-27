#pragma once

#include <tavros/core/math.hpp>

namespace app
{

    struct vertex_type
    {
        tavros::math::vec3 pos;
        tavros::math::vec3 normal;
        tavros::math::vec2 uv;
    };

    extern const vertex_type cube_vertices[24];
    extern const uint32      cube_indices[36];

} // namespace app
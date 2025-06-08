#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    /**
     * Represents the type of a primitive, such as points, lines, or triangles.
     */
    enum class primitive_type : uint8
    {
        points,
        lines,
        line_strip,
        triangles,
        triangle_strip,
        triangle_fan,
    };

} // namespace tavros::renderer

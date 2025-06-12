#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    /**
     * Represents the type of a primitive, such as points, lines, or triangles.
     */
    enum class primitive_type : uint8
    {
        points,         /// Points, each vertex is a point
        lines,          /// Lines, each 2 vertices define a line
        line_strip,     /// Line strip, each vertex is connected to the previous one
        triangles,      /// Triangles, each 3 vertices define a triangle
        triangle_strip, /// Triangle strip each vertex is connected to the previous two
        triangle_fan,   /// Triangle fan, each vertex is connected to the first vertex
    };

} // namespace tavros::renderer

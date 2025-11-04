#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/memory/buffer_span.hpp>
#include <tavros/core/math/vec3.hpp>

namespace tavros::renderer
{

    struct builtin_geometry_vertex
    {
        math::vec3 pos;
        math::vec3 norm;
    };

    struct builtin_geometry_info
    {
        uint32 vertices_count = 0;
        uint32 indices_count = 0;
    };

    class builtin_geometry_generator : core::nonconstructable
    {
    public:
        static builtin_geometry_info cube_info();
        static bool                  gen_cube(core::buffer_span<builtin_geometry_vertex> vertices, core::buffer_span<uint32> indices);

        static builtin_geometry_info icosphere_info(uint32 subdivisions);
        static bool                  gen_icosphere(uint32 subdivisions, core::buffer_span<builtin_geometry_vertex> vertices, core::buffer_span<uint32> indices);
    };

} // namespace tavros::renderer

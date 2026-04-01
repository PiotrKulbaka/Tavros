#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/memory/buffer_span.hpp>
#include <tavros/core/math/vec3.hpp>

namespace tavros::renderer
{

    class geometry_generator : core::nonconstructable
    {
    public:
        struct vertex_t
        {
            math::vec3 pos;
            math::vec3 norm;
        };

        struct geometry_info
        {
            uint32 vertices_count = 0;
            uint32 indices_count = 0;
        };

        struct icosphere_params
        {
            uint32 subdivisions = 0;
        };

        struct cone_params
        {
            float  radius_top = 1.0f;
            float  radius_bottom = 1.0f;
            uint32 segments = 0;
        };

    public:
        static geometry_info cube_info() noexcept;
        static bool          gen_cube(core::buffer_span<vertex_t> vertices, core::buffer_span<uint32> indices);

        static geometry_info icosphere_info(const icosphere_params& params) noexcept;
        static bool          gen_icosphere(const icosphere_params& params, core::buffer_span<vertex_t> vertices, core::buffer_span<uint32> indices);

        static geometry_info cone_info(const cone_params& params) noexcept;
        static bool          gen_cone(const cone_params& params, core::buffer_span<vertex_t> vertices, core::buffer_span<uint32> indices);
    };

} // namespace tavros::renderer

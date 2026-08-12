#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/math/color32.hpp>
#include <tavros/core/geometry/radius4.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>
#include <tavros/renderer/shader_bindings/shader_binding_indices.hpp>

namespace tavros::renderer
{

    struct shape_id
    {
        uint32 index = 0;
    };

    class shape_collection
    {
    public:
        shape_collection() noexcept = default;

        ~shape_collection() noexcept = default;

        shape_id add_line(const math::vec2& from, const math::vec2& to, float thickness = 1.0f, float dash = 0.0f, float gap = 0.0f);

        shape_id add_circle(const math::vec2& center, float radius);
        
        shape_id add_round_rect(const math::vec2& center, const math::vec2& half_sizes, const geometry::radius4& radius = {}, float rot = 0.0f);

        size_t upload_size_bytes() const noexcept;

        rhi::buffer_binding upload(gpu_stream_buffer& sb) const noexcept;

        void clear() noexcept;

    public:
        struct shape_type
        {
            uint32     flags; // shape type
            float      rot;   // ROUND_RECT: rotation angle
            uint32     reserved0;
            uint32     reserved1;
            math::vec4 arg0;  // LINE: xy - start, zw - end; CIRCLE: xy - center, z - radius; ROUND_RECT: xy - center, zw - half sizes;
            math::vec4 arg1;  // LINE: x - thickness, y - dash_size, z - gap_size; ROUND_RECT: x - top-left, y - top-right, z - borrom-right, w - bottom-left;
        };

        struct shape_buffer
        {
            uint32 size = 0; // Size of shapes
            uint32 reserved = 0; // Reserved for future
            // shape_type shapes[]; // In shader
        };

    private:
        core::vector<shape_type> m_shapes;
    };
    
}

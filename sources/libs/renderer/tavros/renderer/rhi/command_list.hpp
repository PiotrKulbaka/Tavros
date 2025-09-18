#pragma once

#include <tavros/renderer/rhi/handle.hpp>

namespace tavros::renderer::rhi
{

    struct texture_copy_region
    {
        // offset x
        uint32 offset_x = 0;

        // for 2D textures offset
        uint32 offset_y = 0;

        // for 3D textures
        uint32 offset_z = 0;

        // Width of the region
        uint32 width = 0;

        // Height of the region
        uint32 height = 0;

        // for 3D textures depth of the region
        uint32 depth = 1;
    };

    class command_list
    {
    public:
        virtual ~command_list() = default;

        // Copy operations
        // virtual void copy_texture_data(texture_handle texture, const texture_copy_region& region, const uint8* pixels, uint32 stride = 0) = 0;
        // virtual void copy_buffer_data(buffer_handle buffer, uint32 offset, const uint8* data, uint32 size) = 0;

        virtual void bind_pipeline(pipeline_handle pipeline) = 0;

        virtual void bind_geometry(geometry_binding_handle geometry_binding) = 0;

        virtual void bind_shader_binding(shader_binding_handle shader_binding) = 0;

        virtual void begin_render_pass(render_pass_handle render_pass, framebuffer_handle framebuffer) = 0;

        virtual void end_render_pass() = 0;

        virtual void draw(uint32 count, uint32 first_vertex = 0) = 0;

        virtual void draw_indexed(uint32 index_count, uint32 first_index = 0, uint32 vertex_offset = 0, uint32 instance_count = 1, uint32 first_instance = 0) = 0;

        virtual void copy_buffer_data(buffer_handle buffer, const void* data, size_t size, size_t offset = 0) = 0;

        virtual void copy_buffer(buffer_handle dst_buffer, buffer_handle src_buffer, size_t size, size_t dst_offset = 0, size_t src_offset = 0) = 0;
    };

} // namespace tavros::renderer::rhi

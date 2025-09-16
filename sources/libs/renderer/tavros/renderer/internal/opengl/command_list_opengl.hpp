#pragma once

#include <tavros/renderer/rhi/command_list.hpp>
#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

namespace tavros::renderer
{

    class command_list_opengl final : public command_list
    {
    public:
        command_list_opengl(graphics_device_opengl* device);
        ~command_list_opengl() override;

        void bind_pipeline(pipeline_handle pipeline) override;

        void bind_geometry(geometry_binding_handle geometry_binding) override;

        void bind_shader_binding(shader_binding_handle shader_binding) override;

        void begin_render_pass(render_pass_handle render_pass, framebuffer_handle framebuffer) override;

        void end_render_pass() override;

        void draw(uint32 count, uint32 first_vertex = 0) override;

        void draw_indexed(uint32 index_count, uint32 first_index = 0, uint32 vertex_offset = 0, uint32 instance_count = 1, uint32 first_instance = 0) override;

        void copy_buffer_data(buffer_handle buffer, const void* data, uint64 size, uint64 offset = 0) override;

        void copy_buffer(buffer_handle dst_buffer, buffer_handle src_buffer, uint64 size, uint64 dst_offset = 0, uint64 src_offset = 0) override;

    private:
        graphics_device_opengl* m_device = nullptr;
        pipeline_handle         m_current_pipeline = {0};
        render_pass_handle      m_current_render_pass = {0};
        framebuffer_handle      m_current_framebuffer = {0};
        geometry_binding_handle m_current_geometry_binding = {0};

        GLuint m_resolve_fbo = 0;
    };

} // namespace tavros::renderer

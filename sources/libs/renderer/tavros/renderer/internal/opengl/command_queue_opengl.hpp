#pragma once

#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

namespace tavros::renderer::rhi
{

    class command_queue_opengl final : public command_queue
    {
    public:
        command_queue_opengl(graphics_device_opengl* device);

        ~command_queue_opengl() override;

        void bind_pipeline(pipeline_handle pipeline) override;

        void bind_geometry(geometry_handle geometry) override;

        void bind_shader_binding(shader_binding_handle shader_binding) override;

        void begin_render_pass(render_pass_handle render_pass, framebuffer_handle framebuffer) override;

        void end_render_pass() override;

        void draw(uint32 vertex_count, uint32 first_vertex = 0, uint32 instance_count = 1, uint32 first_instance = 0) override;

        void draw_indexed(uint32 index_count, uint32 first_index = 0, uint32 vertex_offset = 0, uint32 instance_count = 1, uint32 first_instance = 0) override;

        void signal_fence(fence_handle fence) override;

        void wait_for_fence(fence_handle fence) override;

        void copy_buffer(buffer_handle src_buffer, buffer_handle dst_buffer, size_t size, size_t src_offset = 0, size_t dst_offset = 0) override;

        void copy_buffer_to_texture(buffer_handle src_buffer, texture_handle dst_texture, const texture_copy_region& region) override;

        void copy_texture_to_buffer(texture_handle src_texture, buffer_handle dst_buffer, const texture_copy_region& region) override;

    private:
        graphics_device_opengl* m_device = nullptr;
        pipeline_handle         m_current_pipeline;
        render_pass_handle      m_current_render_pass;
        framebuffer_handle      m_current_framebuffer;
        geometry_handle         m_current_geometry;

        GLuint m_resolve_fbo = 0;
    };

} // namespace tavros::renderer::rhi

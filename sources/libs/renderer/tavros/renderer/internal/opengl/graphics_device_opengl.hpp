#pragma once

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/internal/opengl/device_resources_opengl.hpp>

namespace tavros::renderer::rhi
{
    class gl_command_list;

    class graphics_device_opengl final : public graphics_device
    {
    public:
        graphics_device_opengl();
        ~graphics_device_opengl() override;

        void destroy();

        frame_composer_handle             create_frame_composer(const frame_composer_create_info& info) override;
        void                              destroy_frame_composer(frame_composer_handle composer) override;
        const frame_composer_create_info* get_frame_composer_create_info(frame_composer_handle composer) const noexcept override;
        frame_composer*                   get_frame_composer_ptr(frame_composer_handle composer) override;

        shader_handle         compile_shader(const shader_program_sources& sources) override;
        void                  destroy_shader(shader_handle shader) override;
        const shader_reflect* get_shader_reflect_ptr(shader_handle shader) const noexcept override;

        sampler_handle             create_sampler(const sampler_create_info& info) override;
        void                       destroy_sampler(sampler_handle handle) override;
        const sampler_create_info* get_sampler_create_info(sampler_handle sampler) const noexcept override;

        texture_handle             create_texture(const texture_create_info& info) override;
        void                       destroy_texture(texture_handle handle) override;
        const texture_create_info* get_texture_create_info(texture_handle texture) const noexcept override;

        pipeline_handle             create_pipeline(const pipeline_create_info& info) override;
        void                        destroy_pipeline(pipeline_handle pipeline) override;
        const pipeline_create_info* get_pipeline_create_info(pipeline_handle pipeline) const noexcept override;

        framebuffer_handle             create_framebuffer(const framebuffer_create_info& info) override;
        void                           destroy_framebuffer(framebuffer_handle framebuffer) override;
        const framebuffer_create_info* get_framebuffer_create_info(framebuffer_handle framebuffer) const noexcept override;

        buffer_handle             create_buffer(const buffer_create_info& info) override;
        void                      destroy_buffer(buffer_handle buffer) override;
        const buffer_create_info* get_buffer_create_info(buffer_handle buffer) const noexcept override;

        render_pass_handle             create_render_pass(const render_pass_create_info& info) override;
        void                           destroy_render_pass(render_pass_handle render_pass) override;
        const render_pass_create_info* get_render_pass_create_info(render_pass_handle render_pass) const noexcept override;

        fence_handle create_fence() override;
        void         destroy_fence(fence_handle fence) override;

        bool is_fence_signaled(fence_handle fence) override;
        bool wait_for_fence(fence_handle fence, uint64 timeout_ns = 0xffffffffffffffffui64) override;

        core::buffer_span<uint8> map_buffer(buffer_handle buffer, size_t offset = 0, size_t size = 0) override;
        void                     unmap_buffer(buffer_handle buffer) override;

        device_resources_opengl* get_resources();

    private:
        void init_limits();

        void release_program(gl_program_handle handle) noexcept;

        struct gl_limits
        {
            // Texture info
            uint32 max_2d_texture_size = 0;
            uint32 max_3d_texture_size = 0;
            uint32 max_array_texture_layers = 0;

            // Buffers info
            uint32 max_ubo_size = 0;
            uint32 max_ssbo_size = 0;

            // Vertex attributes info
            uint32 max_vertex_attributes = 0;

            // Framebuffer renderbuffer info
            uint32 max_color_attachmants = 0;
            uint32 max_draw_buffers = 0;
            uint32 max_renderbuffer_size = 0;
            uint32 max_framebuffer_width = 0;
            uint32 max_framebuffer_height = 0;
        };

        device_resources_opengl m_resources;
        gl_limits               m_limits;
    };

} // namespace tavros::renderer::rhi

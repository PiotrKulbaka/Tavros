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

        frame_composer_handle create_frame_composer(
            const frame_composer_create_info& info,
            void*                             native_handle
        ) override;
        void destroy_frame_composer(frame_composer_handle composer) override;

        frame_composer* get_frame_composer_ptr(frame_composer_handle composer) override;

        shader_handle create_shader(const shader_create_info& info) override;

        void destroy_shader(shader_handle shader) override;

        sampler_handle create_sampler(
            const sampler_create_info& info
        ) override;
        void destroy_sampler(sampler_handle handle) override;

        texture_handle create_texture(
            const texture_create_info& info
        ) override;
        void destroy_texture(texture_handle handle) override;

        pipeline_handle create_pipeline(
            const pipeline_create_info&      info,
            core::buffer_view<shader_handle> shaders
        ) override;
        void destroy_pipeline(pipeline_handle pipeline) override;

        framebuffer_handle create_framebuffer(
            const framebuffer_create_info&    info,
            core::buffer_view<texture_handle> color_attachments,
            core::optional<texture_handle>    depth_stencil_attachment = core::nullopt
        ) override;
        void destroy_framebuffer(framebuffer_handle framebuffer) override;

        buffer_handle create_buffer(const buffer_create_info& info) override;
        void          destroy_buffer(buffer_handle buffer) override;

        geometry_handle create_geometry(const geometry_create_info& info) override;
        void            destroy_geometry(geometry_handle geometry) override;

        render_pass_handle create_render_pass(
            const render_pass_create_info&    info,
            core::buffer_view<texture_handle> resolve_textures = core::buffer_view<texture_handle>()
        ) override;
        void destroy_render_pass(render_pass_handle render_pass) override;

        shader_binding_handle create_shader_binding(
            const shader_binding_create_info& info,
            core::buffer_view<texture_handle> textures,
            core::buffer_view<sampler_handle> samplers,
            core::buffer_view<buffer_handle>  buffers
        ) override;
        void destroy_shader_binding(shader_binding_handle shader_binding) override;

        core::buffer_span<uint8> map_buffer(
            buffer_handle buffer,
            size_t        offset = 0,
            size_t        size = 0
        ) override;
        void unmap_buffer(buffer_handle buffer) override;

        device_resources_opengl* get_resources();

    private:
        void init_limits();

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

        core::mallocator        m_internal_allocator;
        device_resources_opengl m_resources;
        gl_limits               m_limits;
    };

} // namespace tavros::renderer::rhi

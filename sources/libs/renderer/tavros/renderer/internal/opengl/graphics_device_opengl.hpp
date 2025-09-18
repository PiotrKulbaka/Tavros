#pragma once

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/internal/opengl/device_resources_opengl.hpp>

namespace tavros::renderer
{
    class gl_command_list;

    class graphics_device_opengl final : public graphics_device
    {
    public:
        graphics_device_opengl();
        ~graphics_device_opengl() override;

        void destroy();

        frame_composer_handle create_frame_composer(
            const frame_composer_info& info,
            void*                      native_handle
        ) override;
        void destroy_frame_composer(frame_composer_handle composer) override;

        frame_composer* get_frame_composer_ptr(frame_composer_handle composer) override;

        shader_handle create_shader(const shader_info& info) override;

        void destroy_shader(shader_handle shader) override;

        sampler_handle create_sampler(
            const sampler_info& info
        ) override;
        void destroy_sampler(sampler_handle handle) override;

        texture_handle create_texture(
            const texture_info& info,
            const uint8*        pixels = nullptr,
            uint32              stride = 0
        ) override;
        void destroy_texture(texture_handle handle) override;

        pipeline_handle create_pipeline(
            const pipeline_info&                  info,
            const core::span<const shader_handle> shaders
        ) override;
        void destroy_pipeline(pipeline_handle pipeline) override;

        framebuffer_handle create_framebuffer(
            const framebuffer_info&                info,
            const core::span<const texture_handle> color_attachments,
            core::optional<texture_handle>         depth_stencil_attachment = core::nullopt
        ) override;
        void destroy_framebuffer(framebuffer_handle framebuffer) override;

        buffer_handle create_buffer(const buffer_info& info) override;
        void          destroy_buffer(buffer_handle buffer) override;

        geometry_binding_handle create_geometry(
            const geometry_binding_info&          info,
            const core::span<const buffer_handle> vertex_buffers,
            core::optional<buffer_handle>         index_buffer = core::nullopt
        ) override;
        void destroy_geometry(geometry_binding_handle geometry_binding) override;

        render_pass_handle create_render_pass(
            const render_pass_info&                info,
            const core::span<const texture_handle> resolve_textures = core::span<const texture_handle>()
        ) override;
        void destroy_render_pass(render_pass_handle render_pass) override;

        shader_binding_handle create_shader_binding(
            const shader_binding_info&             info,
            const core::span<const texture_handle> textures,
            const core::span<const sampler_handle> samplers,
            const core::span<const buffer_handle>  buffers
        ) override;
        virtual void destroy_shader_binding(shader_binding_handle shader_binding) override;


        device_resources_opengl* get_resources();

    private:
        device_resources_opengl m_resources;
    };

} // namespace tavros::renderer

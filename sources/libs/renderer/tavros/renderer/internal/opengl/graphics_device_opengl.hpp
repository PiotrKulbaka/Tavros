#pragma once

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/internal/opengl/device_resources_opengl.hpp>

namespace tavros::renderer
{
    class gl_command_list;

    class graphics_device_opengl final : public tavros::renderer::graphics_device
    {
    public:
        graphics_device_opengl();
        ~graphics_device_opengl() override;

        void destroy();

        virtual swapchain_handle create_swapchain(
            const swapchain_desc& desc,
            void*                 native_handle
        ) override;
        virtual void destroy_swapchain(swapchain_handle swapchain) override;

        virtual swapchain* get_swapchain_ptr_by_handle(swapchain_handle swapchain) override;

        sampler_handle create_sampler(
            const sampler_desc& desc
        ) override;
        void destroy_sampler(sampler_handle handle) override;

        texture_handle create_texture(
            const texture_desc& desc,
            const uint8*        pixels = nullptr,
            uint32 stride = 0
        ) override;
        void destroy_texture(texture_handle handle) override;

        pipeline_handle create_pipeline(
            const pipeline_desc& desc
        ) override;
        void destroy_pipeline(pipeline_handle pipeline) override;

        framebuffer_handle create_framebuffer(
            const framebuffer_desc&                desc,
            const core::span<const texture_handle> color_attachments,
            core::optional<texture_handle>         depth_stencil_attachment = core::nullopt
        ) override;
        void destroy_framebuffer(framebuffer_handle framebuffer) override;

        buffer_handle create_buffer(
            const buffer_desc& desc,
            const uint8* data, uint64 size
        ) override;
        void destroy_buffer(buffer_handle buffer) override;

        geometry_binding_handle create_geometry(
            const geometry_binding_desc&          desc,
            const core::span<const buffer_handle> vertex_buffers,
            core::optional<buffer_handle>         index_buffer = core::nullopt
        ) override;
        void destroy_geometry(geometry_binding_handle geometry_binding) override;

        device_resources_opengl* get_resources();

    private:
        device_resources_opengl m_resources;
    };

} // namespace tavros::renderer

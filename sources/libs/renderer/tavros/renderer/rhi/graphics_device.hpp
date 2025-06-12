#pragma once

#include <tavros/core/containers/sapn.hpp>
#include <tavros/core/optional.hpp>

#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/sampler_desc.hpp>
#include <tavros/renderer/rhi/texture_desc.hpp>
#include <tavros/renderer/rhi/pipeline_desc.hpp>
#include <tavros/renderer/rhi/framebuffer_desc.hpp>
#include <tavros/renderer/rhi/buffer_desc.hpp>
#include <tavros/renderer/rhi/geometry_binding_desc.hpp>

namespace tavros::renderer
{

    class graphics_device
    {
    public:
        virtual ~graphics_device() = default;

        virtual sampler_handle create_sampler(
            const sampler_desc& desc
        ) = 0;
        virtual void destroy_sampler(sampler_handle sampler) = 0;

        virtual texture_handle create_texture(
            const texture_desc& desc,
            const uint8*        pixels = nullptr,
            uint32 stride = 0
        ) = 0;
        virtual void destroy_texture(texture_handle texture) = 0;

        virtual pipeline_handle create_pipeline(
            const pipeline_desc& desc
        ) = 0;
        virtual void destroy_pipeline(pipeline_handle pipeline) = 0;

        virtual framebuffer_handle create_framebuffer(
            const framebuffer_desc&                desc,
            const core::span<const texture_handle> color_attachments,
            core::optional<texture_handle>         depth_stencil_attachment = core::nullopt
        ) = 0;
        virtual void destroy_framebuffer(framebuffer_handle framebuffer) = 0;

        virtual buffer_handle create_buffer(
            const buffer_desc& desc,
            const uint8*       data,
            uint64             size
        ) = 0;
        virtual void destroy_buffer(buffer_handle buffer) = 0;

        virtual geometry_binding_handle create_geometry(
            const geometry_binding_desc&          desc,
            const core::span<const buffer_handle> vertex_buffers,
            core::optional<buffer_handle>         index_buffer = core::nullopt
        ) = 0;
        virtual void destroy_geometry_binding(geometry_binding_handle geometry_binding) = 0;
    };

} // namespace tavros::renderer


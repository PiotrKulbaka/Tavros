#pragma once

#include <tavros/core/containers/sapn.hpp>
#include <tavros/core/optional.hpp>

#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/frame_composer_desc.hpp>
#include <tavros/renderer/rhi/sampler_desc.hpp>
#include <tavros/renderer/rhi/texture_desc.hpp>
#include <tavros/renderer/rhi/pipeline_desc.hpp>
#include <tavros/renderer/rhi/framebuffer_desc.hpp>
#include <tavros/renderer/rhi/buffer_desc.hpp>
#include <tavros/renderer/rhi/geometry_binding_desc.hpp>
#include <tavros/renderer/rhi/render_pass_desc.hpp>
#include <tavros/renderer/rhi/shader_binding_desc.hpp>
#include <tavros/renderer/rhi/shader_info.hpp>

#include <tavros/renderer/rhi/frame_composer.hpp>

namespace tavros::renderer
{

    class graphics_device
    {
    public:
        virtual ~graphics_device() = default;

        virtual frame_composer_handle create_frame_composer(
            const frame_composer_desc& desc,
            void*                      native_handle
        ) = 0;
        virtual void destroy_frame_composer(frame_composer_handle composer) = 0;

        virtual frame_composer* get_frame_composer_ptr(frame_composer_handle composer) = 0;


        virtual shader_handle create_shader(const shader_info& info) = 0;
        virtual void          destroy_shader(shader_handle shader) = 0;


        virtual sampler_handle create_sampler(
            const sampler_desc& desc
        ) = 0;
        virtual void destroy_sampler(sampler_handle sampler) = 0;

        virtual texture_handle create_texture(
            const texture_desc& desc,
            const uint8*        pixels = nullptr,
            uint32              stride = 0
        ) = 0;
        virtual void destroy_texture(texture_handle texture) = 0;

        virtual pipeline_handle create_pipeline(
            const pipeline_desc&                  desc,
            const core::span<const shader_handle> shaders
        ) = 0;
        virtual void destroy_pipeline(pipeline_handle pipeline) = 0;

        virtual framebuffer_handle create_framebuffer(
            const framebuffer_desc&                desc,
            const core::span<const texture_handle> color_attachments,
            core::optional<texture_handle>         depth_stencil_attachment = core::nullopt
        ) = 0;
        virtual void destroy_framebuffer(framebuffer_handle framebuffer) = 0;

        virtual buffer_handle create_buffer(const buffer_desc& desc) = 0;
        virtual void          destroy_buffer(buffer_handle buffer) = 0;

        virtual geometry_binding_handle create_geometry(
            const geometry_binding_desc&          desc,
            const core::span<const buffer_handle> vertex_buffers,
            core::optional<buffer_handle>         index_buffer = core::nullopt
        ) = 0;
        virtual void destroy_geometry(geometry_binding_handle geometry_binding) = 0;

        virtual render_pass_handle create_render_pass(
            const render_pass_desc&                desc,
            const core::span<const texture_handle> resolve_textures = core::span<const texture_handle>()
        ) = 0;
        virtual void destroy_render_pass(render_pass_handle render_pass) = 0;

        virtual shader_binding_handle create_shader_binding(
            const shader_binding_desc&             desc,
            const core::span<const texture_handle> textures,
            const core::span<const sampler_handle> samplers,
            const core::span<const buffer_handle>  buffers
        ) = 0;
        virtual void destroy_shader_binding(shader_binding_handle shader_binding) = 0;
    };

} // namespace tavros::renderer


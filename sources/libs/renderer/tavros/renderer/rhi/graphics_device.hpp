#pragma once

#include <tavros/core/containers/sapn.hpp>
#include <tavros/core/optional.hpp>
#include <tavros/core/memory/memory.hpp>

#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/frame_composer.hpp>
#include <tavros/renderer/rhi/enums.hpp>

#include <tavros/renderer/rhi/frame_composer_create_info.hpp>
#include <tavros/renderer/rhi/sampler_create_info.hpp>
#include <tavros/renderer/rhi/texture_create_info.hpp>
#include <tavros/renderer/rhi/pipeline_create_info.hpp>
#include <tavros/renderer/rhi/framebuffer_create_info.hpp>
#include <tavros/renderer/rhi/buffer_create_info.hpp>
#include <tavros/renderer/rhi/geometry_create_info.hpp>
#include <tavros/renderer/rhi/render_pass_create_info.hpp>
#include <tavros/renderer/rhi/shader_binding_create_info.hpp>
#include <tavros/renderer/rhi/shader_create_info.hpp>

namespace tavros::renderer::rhi
{

    class graphics_device
    {
    public:
        static core::unique_ptr<graphics_device> create(render_backend_type backend);

    public:
        virtual ~graphics_device() = default;

        virtual frame_composer_handle create_frame_composer(
            const frame_composer_create_info& info,
            void*                             native_handle
        ) = 0;
        virtual void destroy_frame_composer(frame_composer_handle composer) = 0;

        virtual frame_composer* get_frame_composer_ptr(frame_composer_handle composer) = 0;


        virtual shader_handle create_shader(const shader_create_info& info) = 0;
        virtual void          destroy_shader(shader_handle shader) = 0;


        virtual sampler_handle create_sampler(
            const sampler_create_info& info
        ) = 0;
        virtual void destroy_sampler(sampler_handle sampler) = 0;

        virtual texture_handle create_texture(
            const texture_create_info& info
        ) = 0;
        virtual void destroy_texture(texture_handle texture) = 0;

        virtual pipeline_handle create_pipeline(
            const pipeline_create_info&           info,
            const core::span<const shader_handle> shaders
        ) = 0;
        virtual void destroy_pipeline(pipeline_handle pipeline) = 0;

        virtual framebuffer_handle create_framebuffer(
            const framebuffer_create_info&         info,
            const core::span<const texture_handle> color_attachments,
            core::optional<texture_handle>         depth_stencil_attachment = core::nullopt
        ) = 0;
        virtual void destroy_framebuffer(framebuffer_handle framebuffer) = 0;

        virtual buffer_handle create_buffer(const buffer_create_info& info) = 0;
        virtual void          destroy_buffer(buffer_handle buffer) = 0;

        virtual geometry_handle create_geometry(
            const geometry_create_info&           info,
            const core::span<const buffer_handle> vertex_buffers,
            core::optional<buffer_handle>         index_buffer = core::nullopt
        ) = 0;
        virtual void destroy_geometry(geometry_handle geometry) = 0;

        virtual render_pass_handle create_render_pass(
            const render_pass_create_info&         info,
            const core::span<const texture_handle> resolve_textures = core::span<const texture_handle>()
        ) = 0;
        virtual void destroy_render_pass(render_pass_handle render_pass) = 0;

        virtual shader_binding_handle create_shader_binding(
            const shader_binding_create_info&      info,
            const core::span<const texture_handle> textures,
            const core::span<const sampler_handle> samplers,
            const core::span<const buffer_handle>  buffers
        ) = 0;
        virtual void destroy_shader_binding(shader_binding_handle shader_binding) = 0;


        /**
         * @brief Map a CPU-visible buffer for read or write access.
         *
         * Returns a pointer to the buffer's memory region accessible by the CPU.
         * If both @p offset and @p size are zero, the entire buffer is mapped.
         *
         * @note Only CPU-visible buffers (e.g., staging or upload) can be mapped.
         *
         * @param buffer Buffer handle to map.
         * @param offset Offset within the buffer to start mapping (in bytes).
         * @param size   Size of the region to map in bytes (0 = entire buffer).
         * @return Pointer to the mapped memory region.
         */
        virtual uint8* map_buffer(buffer_handle buffer, size_t offset = 0, size_t size = 0) = 0;

        /**
         * @brief Unmap a previously mapped buffer.
         *
         * Flushes any pending writes (if required by backend) and invalidates CPU access.
         *
         * @param buffer Buffer handle to unmap.
         */
        virtual void unmap_buffer(buffer_handle buffer) = 0;
    };

} // namespace tavros::renderer::rhi


#pragma once

#include <tavros/core/memory/buffer_view.hpp>
#include <tavros/core/memory/buffer_span.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/optional.hpp>

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

    /**
     * @brief Abstract interface for a graphics device.
     *
     * Provides methods to create, manage, and destroy all GPU resources and rendering objects.
     *
     * A concrete implementation must exist for each supported backend (e.g. OpenGL, Metal, Vulkan, etc.).
     * This interface serves as the main entry point for resource creation and interaction with the GPU.
     */
    class graphics_device
    {
    public:
        /**
         * @brief Create a graphics device for the specified rendering backend.
         *
         * @param backend Rendering backend type (e.g. OpenGL, Metal, Vulkan).
         * @return A unique pointer to the created graphics device.
         */
        static core::unique_ptr<graphics_device> create(render_backend_type backend);

    public:
        virtual ~graphics_device() = default;

        /**
         * @brief Create a frame composer used to manage per-frame rendering.
         *
         * @param info Creation parameters for the frame composer.
         * @param native_handle Optional native handle (e.g., platform-specific window or surface).
         * @return Handle to the created frame composer.
         */
        virtual frame_composer_handle create_frame_composer(
            const frame_composer_create_info& info,
            void*                             native_handle
        ) = 0;

        /**
         * @brief Destroy a previously created frame composer.
         *
         * @param composer Handle to the frame composer to destroy.
         */
        virtual void destroy_frame_composer(frame_composer_handle composer) = 0;

        /**
         * @brief Retrieve a pointer to a frame composer by its handle.
         *
         * @param composer Handle to the frame composer.
         * @return Pointer to the frame composer instance, or nullptr if invalid.
         */
        virtual frame_composer* get_frame_composer_ptr(frame_composer_handle composer) = 0;

        /**
         * @brief Create a shader from provided source or binary.
         *
         * @param info Shader creation parameters.
         * @return Handle to the created shader.
         */
        virtual shader_handle create_shader(const shader_create_info& info) = 0;

        /**
         * @brief Destroy a previously created shader.
         *
         * @param shader Handle to the shader to destroy.
         */
        virtual void destroy_shader(shader_handle shader) = 0;

        /**
         * @brief Create a sampler object used for texture sampling.
         *
         * @param info Sampler creation parameters.
         * @return Handle to the created sampler.
         */
        virtual sampler_handle create_sampler(
            const sampler_create_info& info
        ) = 0;

        /**
         * @brief Destroy a previously created sampler.
         *
         * @param sampler Handle to the sampler to destroy.
         */
        virtual void destroy_sampler(sampler_handle sampler) = 0;

        /**
         * @brief Create a texture resource.
         *
         * @param info Texture creation parameters.
         * @return Handle to the created texture.
         */
        virtual texture_handle create_texture(
            const texture_create_info& info
        ) = 0;

        /**
         * @brief Destroy a previously created texture.
         *
         * @param texture Handle to the texture to destroy.
         */
        virtual void destroy_texture(texture_handle texture) = 0;

        /**
         * @brief Create a graphics pipeline.
         *
         * @param info Pipeline creation parameters.
         * @param shaders List of shader handles used by the pipeline.
         * @return Handle to the created pipeline.
         */
        virtual pipeline_handle create_pipeline(
            const pipeline_create_info&      info,
            core::buffer_view<shader_handle> shaders
        ) = 0;

        /**
         * @brief Destroy a previously created pipeline.
         *
         * @param pipeline Handle to the pipeline to destroy.
         */
        virtual void destroy_pipeline(pipeline_handle pipeline) = 0;

        /**
         * @brief Create a framebuffer for rendering output.
         *
         * @param info Framebuffer creation parameters.
         * @param color_attachments List of color attachments.
         * @param depth_stencil_attachment Optional depth-stencil attachment.
         * @return Handle to the created framebuffer.
         */
        virtual framebuffer_handle create_framebuffer(
            const framebuffer_create_info&    info,
            core::buffer_view<texture_handle> color_attachments,
            core::optional<texture_handle>    depth_stencil_attachment = core::nullopt
        ) = 0;

        /**
         * @brief Destroy a previously created framebuffer.
         *
         * @param framebuffer Handle to the framebuffer to destroy.
         */
        virtual void destroy_framebuffer(framebuffer_handle framebuffer) = 0;

        /**
         * @brief Create a GPU buffer resource.
         *
         * @param info Buffer creation parameters.
         * @return Handle to the created buffer.
         */
        virtual buffer_handle create_buffer(const buffer_create_info& info) = 0;

        /**
         * @brief Destroy a previously created buffer.
         *
         * @param buffer Handle to the buffer to destroy.
         */
        virtual void destroy_buffer(buffer_handle buffer) = 0;

        /**
         * @brief Create a geometry resource (vertex and index buffers).
         *
         * @param info Geometry creation parameters.
         * @return Handle to the created geometry.
         */
        virtual geometry_handle create_geometry(const geometry_create_info& info) = 0;

        /**
         * @brief Destroy a previously created geometry.
         *
         * @param geometry Handle to the geometry to destroy.
         */
        virtual void destroy_geometry(geometry_handle geometry) = 0;

        /**
         * @brief Create a render pass.
         *
         * @param info Render pass creation parameters.
         * @param resolve_textures Optional list of resolve textures.
         * @return Handle to the created render pass.
         */
        virtual render_pass_handle create_render_pass(
            const render_pass_create_info&    info,
            core::buffer_view<texture_handle> resolve_textures = core::buffer_view<texture_handle>()
        ) = 0;

        /**
         * @brief Destroy a previously created render pass.
         *
         * @param render_pass Handle to the render pass to destroy.
         */
        virtual void destroy_render_pass(render_pass_handle render_pass) = 0;

        /**
         * @brief Create a shader binding set (descriptor set / resource group).
         *
         * @param info Shader binding creation parameters.
         * @param textures List of bound textures.
         * @param samplers List of bound samplers.
         * @param buffers  List of bound buffers.
         * @return Handle to the created shader binding.
         */
        virtual shader_binding_handle create_shader_binding(
            const shader_binding_create_info& info,
            core::buffer_view<texture_handle> textures,
            core::buffer_view<sampler_handle> samplers,
            core::buffer_view<buffer_handle>  buffers
        ) = 0;

        /**
         * @brief Destroy a previously created shader binding.
         *
         * @param shader_binding Handle to the shader binding to destroy.
         */
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
        virtual core::buffer_span<uint8> map_buffer(buffer_handle buffer, size_t offset = 0, size_t size = 0) = 0;

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


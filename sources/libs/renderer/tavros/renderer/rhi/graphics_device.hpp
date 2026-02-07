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
#include <tavros/renderer/rhi/render_pass_create_info.hpp>
#include <tavros/renderer/rhi/shader_binding_create_info.hpp>
#include <tavros/renderer/rhi/shader_create_info.hpp>

#include <type_traits>

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
         * @return Handle to the created frame composer.
         */
        virtual frame_composer_handle create_frame_composer(const frame_composer_create_info& info) = 0;

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
        virtual sampler_handle create_sampler(const sampler_create_info& info) = 0;

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
        virtual texture_handle create_texture(const texture_create_info& info) = 0;

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
         * @return Handle to the created pipeline.
         */
        virtual pipeline_handle create_pipeline(const pipeline_create_info& info) = 0;

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
         * @return Handle to the created framebuffer.
         */
        virtual framebuffer_handle create_framebuffer(const framebuffer_create_info& info) = 0;

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
         * @brief Create a render pass.
         *
         * @param info Render pass creation parameters.
         * @return Handle to the created render pass.
         */
        virtual render_pass_handle create_render_pass(const render_pass_create_info& info) = 0;

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
         * @return Handle to the created shader binding.
         */
        virtual shader_binding_handle create_shader_binding(const shader_binding_create_info& info) = 0;

        /**
         * @brief Destroy a previously created shader binding.
         *
         * @param shader_binding Handle to the shader binding to destroy.
         */
        virtual void destroy_shader_binding(shader_binding_handle shader_binding) = 0;

        /**
         * @brief Create a GPU fence object used for synchronization.
         *
         * @return Handle to the created fence.
         */
        virtual fence_handle create_fence() = 0;

        /**
         * @brief Destroy a previously created fence.
         *
         * @param fence Handle to the fence to destroy.
         */
        virtual void destroy_fence(fence_handle fence) = 0;

        /**
         * @brief Check if the fence has been signaled by the GPU.
         *
         * @param fence Handle to the fence to query.
         * @return True if the fence is signaled, false otherwise.
         */
        virtual bool is_fence_signaled(fence_handle fence) = 0;

        /**
         * @brief Wait for the fence to be signaled by the GPU.
         *
         * @param fence Handle to the fence to wait on.
         * @param timeout_ns Maximum time to wait in nanoseconds. Defaults to maximum value (infinite wait).
         * @return True if the fence was signaled within the timeout, false if the timeout expired.
         */
        virtual bool wait_for_fence(fence_handle fence, uint64 timeout_ns = 0xffffffffffffffffui64) = 0;

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

        /**
         * @brief Safely destroys a GPU resource handle.
         *
         * Invokes the appropriate destroy function for the given handle type and resets the handle
         * to an invalid state.
         *
         * @param h Reference to the handle to destroy.
         */
        template<class T>
        void safe_destroy(T& h)
        {
            if (!h.valid()) {
                return;
            }

            if constexpr (std::is_same_v<T, frame_composer_handle>) {
                destroy_frame_composer(h);
            } else if constexpr (std::is_same_v<T, sampler_handle>) {
                destroy_sampler(h);
            } else if constexpr (std::is_same_v<T, texture_handle>) {
                destroy_texture(h);
            } else if constexpr (std::is_same_v<T, pipeline_handle>) {
                destroy_pipeline(h);
            } else if constexpr (std::is_same_v<T, framebuffer_handle>) {
                destroy_framebuffer(h);
            } else if constexpr (std::is_same_v<T, buffer_handle>) {
                destroy_buffer(h);
            } else if constexpr (std::is_same_v<T, render_pass_handle>) {
                destroy_render_pass(h);
            } else if constexpr (std::is_same_v<T, shader_binding_handle>) {
                destroy_shader_binding(h);
            } else if constexpr (std::is_same_v<T, shader_handle>) {
                destroy_shader(h);
            } else if constexpr (std::is_same_v<T, fence_handle>) {
                destroy_fence(h);
            } else {
                static_assert(false, "safe_destroy not implemented for this handle type");
            }

            h = {};
        }
    };

} // namespace tavros::renderer::rhi


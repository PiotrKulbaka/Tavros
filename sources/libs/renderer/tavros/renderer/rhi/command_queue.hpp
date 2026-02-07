#pragma once

#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/enums.hpp>
#include <tavros/renderer/rhi/structs.hpp>
#include <tavros/core/memory/buffer_view.hpp>

namespace tavros::renderer::rhi
{

    /**
     * @brief Low-level abstraction for recording GPU commands.
     *
     * This interface represents a command queue — a sequence of GPU operations such as
     * binding resources, performing draw calls, and executing data transfers.
     * Command queues are designed to be recorded on multiple threads and submitted
     * later for execution through a frame_composer.
     *
     * Each command queue encapsulates its own GPU state and does not execute commands
     * immediately; instead, commands are queued until the queue is submitted.
     */
    class command_queue
    {
    public:
        virtual ~command_queue() = default;

        /**
         * @brief Begins recording commands into the command queue.
         *
         * Must be called before issuing any rendering or compute commands.
         * Once begun, commands can be recorded until @ref end() is called.
         * Command queues cannot be executed until they have been properly ended.
         */
        virtual void begin() = 0;

        /**
         * @brief Ends command recording.
         *
         * Finalizes the recorded command sequence, making it ready for submission
         * to a command queue or execution context. After calling this method,
         * no further commands can be recorded until @ref begin() is called again.
         *
         * This must be called before submitting the command buffer for execution.
         */
        virtual void end() = 0;

        /**
         * @brief Bind a graphics pipeline.
         *
         * Associates the given pipeline state object with the command queue.
         * All subsequent draw commands will use this pipeline until another one is bound.
         *
         * @param pipeline Pipeline handle to bind.
         */
        virtual void bind_pipeline(pipeline_handle pipeline) = 0;

        virtual void bind_vertex_buffers(core::buffer_view<bind_buffer_info> buffers) = 0;

        virtual void bind_index_buffer(buffer_handle buffer, index_buffer_format format) = 0;

        /**
         * @brief Bind shader resources to the current pipeline.
         *
         * Binds textures, uniform buffers, and other shader-accessible resources
         * described by the given shader binding object.
         *
         * @param shader_binding Handle to the shader binding to use.
         */
        virtual void bind_shader_binding(shader_binding_handle shader_binding) = 0;

        /**
         * @brief Begin a render pass.
         *
         * Starts rendering into the given framebuffer using the specified render pass.
         * Must be called before issuing any draw calls.
         *
         * @param render_pass Render pass handle defining attachments and load/store operations.
         * @param framebuffer Framebuffer handle specifying color/depth targets.
         */
        virtual void begin_render_pass(render_pass_handle render_pass, framebuffer_handle framebuffer) = 0;

        /**
         * @brief End the current render pass.
         *
         * Finalizes all rendering commands for the current pass.
         * After this call, draw commands are no longer valid until a new render pass begins.
         */
        virtual void end_render_pass() = 0;

        /**
         * @brief Sets the active viewport for subsequent rendering commands.
         *
         * Defines the transformation from normalized device coordinates (NDC)
         * to framebuffer coordinates.
         *
         * Must be called within a render pass before issuing draw commands.
         *
         * @param viewport Viewport parameters, including position, size, and depth range.
         */
        virtual void set_viewport(const viewport_info& viewport) = 0;

        /**
         * @brief Sets the active scissor rectangle for subsequent rendering commands.
         *
         * The scissor rectangle limits rendering to a specific region of the framebuffer.
         * Fragments outside of the defined rectangle are discarded after rasterization.
         *
         * Must be called within a render pass before issuing draw commands.
         *
         * @param scissor Scissor rectangle parameters, defining the clipping region in pixels.
         */
        virtual void set_scissor(const scissor_info& scissor) = 0;

        /**
         * @brief Issue a non-indexed draw command.
         *
         * Draws primitives using currently bound geometry and pipeline.
         * Must be called within a render pass.
         *
         * @param vertex_count   Number of vertices to draw.
         * @param first_vertex   Index of the first vertex to draw.
         * @param instance_count Number of instances to draw (default = 1).
         * @param first_instance Index of the first instance (default = 0).
         */
        virtual void draw(uint32 vertex_count, uint32 first_vertex = 0, uint32 instance_count = 1, uint32 first_instance = 0) = 0;

        /**
         * @brief Issue an indexed draw command.
         *
         * Draws primitives using the bound index buffer and pipeline.
         * Must be called within a render pass.
         *
         * @param index_count    Number of indices to draw.
         * @param first_index    Index of the first index to draw.
         * @param vertex_offset  Value added to each index before fetching the vertex.
         * @param instance_count Number of instances to draw (default = 1).
         * @param first_instance Index of the first instance (default = 0).
         */
        virtual void draw_indexed(uint32 index_count, uint32 first_index = 0, uint32 vertex_offset = 0, uint32 instance_count = 1, uint32 first_instance = 0) = 0;

        /**
         * @brief Signal a fence from the GPU, marking it as completed when reached in the command queue.
         *
         * @param fence Handle to the fence to signal.
         */
        virtual void signal_fence(fence_handle fence) = 0;

        /**
         * @brief Insert a wait on a fence into the GPU command queue. The GPU will pause execution
         *        until the specified fence is signaled.
         *
         * @param fence Handle to the fence to wait for.
         */
        virtual void wait_for_fence(fence_handle fence) = 0;

        /**
         * @brief Copy data between two buffers.
         *
         * Performs a raw memory copy operation between GPU buffers.
         *
         * @param src_buffer Source buffer handle.
         * @param dst_buffer Destination buffer handle.
         * @param size       Number of bytes to copy.
         * @param src_offset Offset within the source buffer in bytes.
         * @param dst_offset Offset within the destination buffer in bytes.
         */
        virtual void copy_buffer(buffer_handle src_buffer, buffer_handle dst_buffer, size_t size, size_t src_offset = 0, size_t dst_offset = 0) = 0;

        /**
         * @brief Copy data from a buffer to a texture.
         *
         * Uploads texture data from a CPU-visible buffer into a GPU texture.
         * Supports partial updates, mipmap levels, layers, and custom buffer layouts.
         *
         * @param src_buffer  Source buffer handle containing image data.
         * @param dst_texture Destination texture handle.
         * @param region      Region of the texture to update, including mip level, layer, offsets,
         *                    dimensions, and buffer layout parameters (buffer_offset, row length, etc.).
         *
         * @note For 2D textures or cube faces, `region.depth` should be 1 and `z_offset` should be 0.
         * @note For 3D textures, `depth` specifies the number of slices and `z_offset` specifies the starting slice.
         * @note `buffer_row_length` of 0 means the buffer rows are tightly packed (row length equals `region.width`).
         */
        virtual void copy_buffer_to_texture(buffer_handle src_buffer, texture_handle dst_texture, const texture_copy_region& region) = 0;

        /**
         * @brief Copies pixel data from a texture to a buffer.
         *
         * Transfers image data from a specified texture (or one of its array layers / mip levels)
         * into a destination buffer. Useful for readback operations, saving rendered images,
         * or performing CPU-side image processing.
         *
         * @param src_texture Handle to the source texture to read from.
         * @param dst_buffer  Handle to the destination buffer where the data will be written.
         * @param region      Region of the texture to copy, including mip level, layer, offsets,
         *                    dimensions, and buffer layout parameters (buffer_offset, row length, etc.).
         *
         * @note For 2D textures or cube faces, `region.depth` should be 1 and `z_offset` should be 0.
         * @note For 3D textures, `depth` specifies the number of slices and `z_offset` specifies the starting slice.
         * @note `buffer_row_length` of 0 means the buffer rows are tightly packed (row length equals `region.width`).
         */
        virtual void copy_texture_to_buffer(texture_handle src_texture, buffer_handle dst_buffer, const texture_copy_region& region) = 0;
    };

} // namespace tavros::renderer::rhi

#pragma once
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/gpu_stage_buffer.hpp>
#include <tavros/core/containers/vector.hpp>

namespace tavros::renderer
{

    /**
     * @brief Manages CPU-to-GPU data uploads through persistent staging buffers.
     *
     * The upload context provides transient regions of CPU-visible staging memory
     * that can be used to upload resources such as textures and buffers to the GPU.
     * Upload allocations are valid until the next call to @ref flush().
     *
     * Internally, the context manages one or more staging buffers and synchronizes
     * reuse of their memory using a fence.
     *
     * This class is not thread-safe.
     */
    class upload_context : core::noncopyable
    {
    public:
        /**
         * @brief Default size of the primary staging buffer.
         */
        static constexpr auto k_stage_buffer_size = 64_mib;

        /**
         * @brief Represents a transient upload allocation.
         *
         * The returned memory region can be filled by the CPU and subsequently
         * used as the source of copy commands recorded on the associated command
         * queue.
         */
        struct upload_batch
        {
            /// Writable region of staging memory.
            gpu_buffer_view<uint8> view;

            /// Command queue that must be used to record the upload commands.
            rhi::command_queue* queue;
        };

    public:
        /**
         * @brief Creates an upload context.
         *
         * Allocates the staging resources required for resource uploads.
         *
         * @param gdevice Graphics device used to create GPU resources.
         */
        explicit upload_context(rhi::graphics_device* gdevice) noexcept;

        /**
         * @brief Destroys the upload context and releases all owned resources.
         */
        ~upload_context() noexcept;

        /**
         * @brief Waits for all pending uploads and recycles staging memory.
         *
         * This function ensures that all GPU operations referencing the staging
         * buffers have completed, making their memory available for subsequent
         * allocations.
         */
        void flush() noexcept;

        /**
         * @brief Allocates a transient region of staging memory.
         *
         * The returned allocation remains valid until the next call to
         * @ref flush().
         *
         * @param size Size of the requested allocation in bytes.
         *
         * @return Upload allocation and the command queue associated with it.
         */
        [[nodiscard]] upload_batch slice(size_t size) noexcept;

    private:
        rhi::graphics_device* m_gdevice = nullptr;
        rhi::command_queue*   m_current_upload_queue = nullptr;
        rhi::fence_handle     m_fence;
        gpu_stage_buffer      m_stage_buffer;
        gpu_stage_buffer      m_large_stage_buffer;
    };

} // namespace tavros::renderer
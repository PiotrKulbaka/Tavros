#pragma once

#include <tavros/renderer/gpu_persistent_buffer.hpp>

namespace tavros::renderer
{

    /**
     * @brief CPU-to-GPU staging buffer for one-time or infrequent uploads.
     *
     * Provides a linear allocator over a persistently mapped staging buffer.
     * Intended for uploading static or infrequently updated data to the GPU:
     * - Textures
     * - Static mesh geometry
     * - Any data that requires a CPU-to-GPU transfer via command queue
     *
     * Typical usage:
     * @code
     * gpu_stage_buffer staging(device, 128_MiB, rhi::buffer_usage::stage);
     *
     * auto slice = staging.slice<uint8>(image.size_bytes());
     * slice.copy_from(image.pixels(), image.size_bytes());
     *
     * cmd->copy_buffer_to_texture(slice.handle(), texture, region);
     * cmd->signal_fence(fence);
     * composer.submit(cmd);
     *
     * device.wait_for_fence(fence);
     * staging.reset();
     * @endcode
     *
     * @note Not copyable.
     */
    class gpu_stage_buffer : public basic_gpu_persistent_buffer
    {
    public:
        /** Default constructor. */
        explicit gpu_stage_buffer() noexcept = default;

        /** @brief Default destructor. */
        ~gpu_stage_buffer() noexcept = default;

        /**
         * @brief Initializes a staging buffer and maps it persistently.
         *
         * @param gdevice  Non-owning pointer to the graphics device. Must outlive this buffer.
         * @param size     Total buffer size in bytes.
         */
        void init(rhi::graphics_device* gdevice, size_t size)
        {
            destroy_all();
            internal_init(gdevice, size, rhi::buffer_usage::stage, rhi::buffer_access::cpu_to_gpu);
        }

        void shutdown()
        {
            destroy_all();
        }
    };

} // namespace tavros::renderer

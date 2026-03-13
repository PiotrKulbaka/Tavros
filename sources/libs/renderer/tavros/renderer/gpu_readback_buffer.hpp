#pragma once

#include <tavros/renderer/gpu_persistent_buffer.hpp>

namespace tavros::renderer
{

    /**
     * @brief GPU-to-CPU readback buffer for retrieving data from the GPU.
     *
     * Provides a linear allocator over a persistently mapped staging buffer
     * with gpu_to_cpu access. Intended for reading data produced by the GPU
     * back to the CPU:
     * - Screenshots and frame captures
     * - Compute shader results
     * - Occlusion query results
     * - Any GPU-written data that needs to be inspected on the CPU
     *
     * Typical usage:
     * @code
     * gpu_readback_buffer readback(device, 64_MiB);
     *
     * auto slice = readback.slice<uint8>(width * height * 4);
     *
     * cmd->copy_texture_to_buffer(texture, slice, region);
     * cmd->signal_fence(fence);
     * composer.submit(cmd);
     *
     * device.wait_for_fence(fence);
     *
     * // Safe to read slice.data() here
     * process_pixels(slice.data(), width, height);
     *
     * readback.reset();
     * @endcode
     *
     * @note Not copyable.
     * @note slice.data() must only be read after waiting on the associated fence.
     *       Reading before the GPU has finished writing yields undefined behaviour.
     * @note reset() must only be called after the CPU has finished reading all slices.
     */
    class gpu_readback_buffer : public basic_gpu_persistent_buffer
    {
    public:
        /** Default constructor. */
        explicit gpu_readback_buffer() noexcept = default;

        /** @brief Default destructor. */
        ~gpu_readback_buffer() noexcept = default;

        /**
         * @brief Initializes a readback buffer and maps it persistently.
         *
         * @param gdevice  Non-owning pointer to the graphics device. Must outlive this buffer.
         * @param size     Total buffer size in bytes.
         */
        void init(rhi::graphics_device* gdevice, size_t size)
        {
            destroy_all();
            internal_init(gdevice, size, rhi::buffer_usage::stage, rhi::buffer_access::gpu_to_cpu);
        }

        void shutdown()
        {
            destroy_all();
        }
    };

} // namespace tavros::renderer

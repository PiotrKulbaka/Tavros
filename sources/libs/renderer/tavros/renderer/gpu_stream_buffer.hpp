#pragma once

#include <tavros/renderer/gpu_persistent_buffer.hpp>

namespace tavros::renderer
{

    /**
     * @brief Persistent CPU-to-GPU streaming buffer.
     *
     * gpu_stream_buffer provides a linear allocator over a persistently mapped GPU buffer.
     * It is intended for frequently updated per-frame data such as:
     * - Uniform buffers
     * - Dynamic vertex or index data
     *
     * Memory is allocated sequentially and reset every frame via begin_frame().
     *
     * @note This class is non-copyable.
     * @note Not thread-safe.
     */
    class gpu_stream_buffer : public basic_gpu_persistent_buffer
    {
    public:
        /** Default constructor. */
        explicit gpu_stream_buffer() = default;

        /** @brief Default destructor. */
        ~gpu_stream_buffer() noexcept = default;

        /**
         * @brief Initializes a streaming GPU buffer and maps it persistently.
         *
         * @param gdevice  Non-owning pointer to the graphics device. Must outlive this buffer.
         * @param size     Total buffer size in bytes.
         */
        void init(rhi::graphics_device* gdevice, size_t size, rhi::buffer_usage usage)
        {
            destroy_all();
            internal_init(gdevice, size, usage, rhi::buffer_access::cpu_to_gpu);
        }

        void shutdown()
        {
            destroy_all();
        }
    };

} // namespace tavros::renderer

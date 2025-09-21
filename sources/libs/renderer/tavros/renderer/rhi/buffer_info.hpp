#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer::rhi
{

    /**
     * Describes the usage of a buffer
     */
    enum class buffer_usage : uint8
    {
        stage,   /// Buffer used for data transfer CPU to GPU
        index,   /// Buffer used for index data (element array)
        vertex,  /// Buffer used for vertex attributes
        uniform, /// Buffer used for uniform / constant data
    };

    /**
     * Describes the access mode for a buffer
     */
    enum class buffer_access : uint8
    {
        cpu_to_gpu, /// CPU writes to the buffer, GPU reads from it (dynamic upload)
        gpu_only,   /// Buffer accessible only by the GPU (immutable or GPU-local)
        gpu_to_cpu, /// GPU writes to the buffer, CPU reads from it (readback)
    };

    /**
     * Describes the properties of a buffer
     */
    struct buffer_info
    {
        /// Buffer size in bytes
        size_t size = 0;

        /// Buffer usage
        buffer_usage usage = buffer_usage::stage;

        /// Access pattern indicating who reads/writes this buffer
        buffer_access access = buffer_access::cpu_to_gpu;
    };

} // namespace tavros::renderer::rhi

#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    /**
     * Describes the usage of a buffer
     */
    enum class buffer_usage : uint8
    {
        index,   /// Buffer used for index data (element array)
        vertex,  /// Buffer used for vertex attributes
        uniform, /// Buffer used for uniform / constant data
    };

    /**
     * Describes the access mode for a buffer
     */
    enum class buffer_access : uint8
    {
        gpu_only,   /// Buffer accessible only by the GPU (immutable or GPU-local)
        cpu_to_gpu, /// CPU writes to the buffer, GPU reads from it (dynamic upload)
        gpu_to_cpu, /// GPU writes to the buffer, CPU reads from it (readback)
    };

    /**
     * Describes the properties of a buffer
     */
    struct buffer_desc
    {
        /// Buffer size in bytes
        uint64 size = 0;

        /// Buffer usage
        /// Used only if access is gpu_only or gpu_to_cpu
        buffer_usage usage = buffer_usage::index;

        /// Access pattern indicating who reads/writes this buffer
        buffer_access access = buffer_access::gpu_only;
    };

} // namespace tavros::renderer

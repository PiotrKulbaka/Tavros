#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    enum class buffer_usage : uint8
    {
        index,
        vertex,
        uniform,
    };

    enum class buffer_access : uint8
    {
        gpu_only,   // Only the GPU can access the buffer
        cpu_to_gpu, // CPU can write to the buffer and GPU can read
        gpu_to_cpu, // GPU can write to the buffer and CPU can read
    };


    struct buffer_desc
    {
        buffer_desc() = default;

        buffer_desc& set_usage(buffer_usage usage)
        {
            this->usage = usage;
            return *this;
        }

        buffer_desc& set_access(buffer_access access)
        {
            this->access = access;
            return *this;
        }

        buffer_desc& set_size(uint32 size)
        {
            this->size = size;
            return *this;
        }

        buffer_usage  usage = buffer_usage::index;      // Buffer usage
        buffer_access access = buffer_access::gpu_only; // Buffer access mode
        uint32        size = 0;                         // Buffer size in bytes
    };

} // namespace tavros::renderer

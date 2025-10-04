#pragma once

#include <tavros/renderer/rhi/enums.hpp>

namespace tavros::renderer::rhi
{

    /**
     * Describes the properties of a buffer
     */
    struct buffer_create_info
    {
        /// Buffer size in bytes
        size_t size = 0;

        /// Buffer usage
        buffer_usage usage = buffer_usage::stage;

        /// Access pattern indicating who reads/writes this buffer
        buffer_access access = buffer_access::cpu_to_gpu;
    };

} // namespace tavros::renderer::rhi

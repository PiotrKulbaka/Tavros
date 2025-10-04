#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/renderer/rhi/limits.hpp>
#include <tavros/renderer/rhi/enums.hpp>

namespace tavros::renderer::rhi
{

    /**
     * Describes how a single vertex buffer is bound to the GPU
     */
    struct buffer_layout
    {
        /// Index of the vertex buffer (as provided when creating the geometry binding)
        uint32 buffer_index = 0;

        /// Offset in bytes from the start of the GPU buffer to the beginning of the vertex data
        uint32 base_offset = 0;

        /// Stride in bytes between consecutive vertices in the buffer
        uint32 stride = 0;
    };

    /**
     * Describes how a single vertex attribute is sourced from a vertex buffer
     */
    struct attribute_binding
    {
        /// Index of the buffer binding that contains this attribute (refers to geometry_info::buffer_layouts)
        uint32 buffer_layout_index = 0;

        /// Offset in bytes from the start of the vertex to this attribute
        uint32 offset = 0;

        /// Specifies how often this attribute advances per instance when rendering with instancing
        /// - 0: attribute is a per-vertex value (changes every vertex)
        /// - 1: attribute is a per-instance value (changes once per instance)
        /// - N (>1): attribute is reused for N consecutive instances before advancing
        uint32 instance_divisor = 0;

        /// Description of the vertex attribute (format, components, etc.)
        vertex_attribute attribute;
    };

    /**
     * Describes the layout of vertex attributes and their source buffers
     * This structure is used by the GPU to correctly interpret vertex data from one or more buffers
     */
    struct geometry_create_info
    {
        /// /// Array of buffer bindings describing physical vertex buffers
        core::static_vector<buffer_layout, k_max_vertex_buffers> buffer_layouts;

        /// Array of attribute bindings describing how vertex attributes are read from buffers
        core::static_vector<attribute_binding, k_max_vertex_attributes> attribute_bindings;

        /// Whether this geometry uses an index buffer
        bool has_index_buffer = false;

        /// Format of the index buffer, if present
        index_buffer_format index_format = index_buffer_format::u16;
    };

} // namespace tavros::renderer::rhi

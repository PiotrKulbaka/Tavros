#pragma once

#include <tavros/core/types.hpp>
#include <tavros/renderer/rhi/vertex_layout.hpp>

namespace tavros::renderer
{

    /**
     * Specifies the format of the index buffer
     */
    enum class index_buffer_format : uint8
    {
        u16, /// 16-bit unsigned integer
        u32, /// 32-bit unsigned integer
    };

    /**
     * Defines how a single vertex attribute is bound t
     */
    struct vertex_buffer_mapping
    {
        /// Binding index
        uint32 binding = 0;

        /// Byte offset from the start of the vertex to this attribute
        uint32 offset = 0;

        /// Stride of the vertex buffer
        uint32 stride = 0;
    };

    /**
     * Defines how a buffer is mapped to a specific attribute
     */
    struct vertex_attribute_mapping
    {
        /// Index of the vertex buffer
        uint32 buffer_index = 0;

        /// Local offset of the attribute in the buffer
        uint32 offset = 0;
    };

    /**
     * Describes the layout of vertex attributes and how they are sourced from bound vertex buffers
     * This structure is used by the GPU to correctly interpret vertex data from multiple (or one) buffers
     */
    struct geometry_binding_desc
    {
        /// Describes how individual vertex attributes are laid out
        vertex_layout layout;

        /// Mapping from vertex attributes to specified vertex buffers
        core::static_vector<vertex_attribute_mapping, k_max_vertex_attributes> attribute_mapping;

        /// Mapping buffers
        core::static_vector<vertex_buffer_mapping, k_max_vertex_attributes> buffer_mapping;

        /// Whether this geometry uses an index buffer
        bool has_index_buffer = 0;

        /// Format of the index buffer (if present)
        index_buffer_format index_format = index_buffer_format::u16;
    };

} // namespace tavros::renderer

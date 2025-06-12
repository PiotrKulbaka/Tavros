#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/renderer/rhi/limits.hpp>

namespace tavros::renderer
{

    /**
     * Describes the data format of a single vertex attribute component
     * Defines how the GPU interprets raw vertex data in memory
     */
    enum class attribute_format : uint8
    {
        u8,  /// Unsigned 8-bit integer
        i8,  /// Signed 8-bit integer
        u16, /// Unsigned 16-bit integer
        i16, /// Signed 16-bit integer
        u32, /// Unsigned 32-bit integer
        i32, /// Signed 32-bit integer
        f16, /// 16-bit half-precision float
        f32, /// 32-bit single-precision float (IEEE 754)
    };

    /**
     * Defines a single vertex attribute within a vertex buffer layout
     * Specifies how one attribute (e.g., position, color, normal) is stored and interpreted
     */
    struct vertex_attribute
    {
        /// Number of components (e.g. 1 = scalar, 2 = vec2, 3 = vec3, 4 = vec4)
        uint8 size = 1;

        /// Format of each component
        attribute_format format = attribute_format::f32;

        /// Whether integer data should be normalized to [0,1] or [-1,1] (only applicable for integer types)
        bool normalize = false;
    };

    /**
     * Describes the full layout of vertex data within a vertex buffer
     * This layout is used by the GPU to interpret vertex input correctly in the vertex shader
     */
    struct vertex_layout
    {
        /// List of vertex attributes
        core::static_vector<vertex_attribute, k_max_vertex_attributes> attributes;
    };

} // namespace tavros::renderer

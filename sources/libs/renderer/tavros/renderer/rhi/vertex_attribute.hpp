#pragma once

#include <tavros/renderer/rhi/enums.hpp>

namespace tavros::renderer::rhi
{

    /**
     * Defines a single vertex attribute within a vertex buffer layout
     * Specifies how one attribute (e.g., position, color, normal) is stored and interpreted
     */
    struct vertex_attribute
    {
        /// Type of attribute
        attribute_type type = attribute_type::scalar;

        /// Format of each component
        attribute_format format = attribute_format::f32;

        /// Whether integer data should be normalized to [0,1] or [-1,1] (only applicable for integer types)
        bool normalize = false;

        /// Attribute location in the shader
        uint32 location = 0;
    };

} // namespace tavros::renderer::rhi

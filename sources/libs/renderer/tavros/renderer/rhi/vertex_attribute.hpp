#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer::rhi
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
        f64, /// 64-bit double-precision float
    };

    /**
     * Describes the semantic type of a vertex attribute
     * Defines how many components form the attribute and whether it represents a vector or a matrix
     */
    enum class attribute_type : uint8
    {
        scalar, /// Single component (float, int, uint, etc.)
        vec2,   /// 2-component vector
        vec3,   /// 3-component vector
        vec4,   /// 4-component vector

        mat2,   /// 2x2 matrix (2 vectors of 2 components)
        mat2x3, /// 2x3 matrix (2 vectors of 3 components)
        mat2x4, /// 2x4 matrix (2 vectors of 4 components)

        mat3x2, /// 3x2 matrix (3 vectors of 2 components)
        mat3,   /// 3x3 matrix (3 vectors of 3 components)
        mat3x4, /// 3x4 matrix (3 vectors of 4 components)

        mat4x2, /// 4x2 matrix (4 vectors of 2 components)
        mat4x3, /// 4x3 matrix (4 vectors of 3 components)
        mat4,   /// 4x4 matrix (4 vectors of 4 components)
    };

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

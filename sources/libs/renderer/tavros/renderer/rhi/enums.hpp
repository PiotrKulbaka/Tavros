#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer::rhi
{

    /**
     * @brief Specifies how a buffer is intended to be used.
     */
    enum class buffer_usage : uint8
    {
        stage,    /// Buffer used for data transfer between CPU and GPU
        index,    /// Buffer used for index data (element array)
        vertex,   /// Buffer used for vertex attribute data
        constant, /// Buffer used for constant (read-only) shader data
        storage,  /// Buffer used for read-write shader data
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
     * Specifies the format of the index buffer
     */
    enum class index_buffer_format : uint8
    {
        u16, /// 16-bit unsigned integer
        u32, /// 32-bit unsigned integer
    };


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
     * Represents the type of a primitive, such as points, lines, or triangles.
     */
    enum class primitive_topology : uint8
    {
        points,         /// Points, each vertex is a point
        lines,          /// Lines, each 2 vertices define a line
        line_strip,     /// Line strip, each vertex is connected to the previous one
        triangles,      /// Triangles, each 3 vertices define a triangle
        triangle_strip, /// Triangle strip each vertex is connected to the previous two
        triangle_fan,   /// Triangle fan, each vertex is connected to the first vertex
    };

    /**
     * Defines comparison functions used in operations such as depth testing or sampler comparison.
     * These functions determine whether a pixel or sampled value passes a comparison test.
     */
    enum class compare_op : uint8
    {
        off,           /// Comparison is disabled; used for samplers where depth comparison is not needed
        less,          /// Passes if the incoming value is less than the stored value
        equal,         /// Passes if the incoming value is equal to the stored value
        less_equal,    /// Passes if the incoming value is less than or equal to the stored value
        greater,       /// Passes if the incoming value is greater than the stored value
        greater_equal, /// Passes if the incoming value is greater than or equal to the stored value
        not_equal,     /// Passes if the incoming value is not equal to the stored value
        always,        /// Always passes the comparison test
    };

    /**
     * Specifies the stencil operation to be performed during rendering
     * This operation is applied to the stencil buffer value at the current pixel location
     */
    enum class stencil_op : uint8
    {
        keep,            /// Keep the current value in the stencil buffer unchanged
        zero,            /// Set the value in the stencil buffer to 0
        replace,         /// Replace the value in the stencil buffer with the reference value
        increment_clamp, /// Increment the value, but not above 0xFF
        decrement_clamp, /// Decrement the value, but not below 0
        invert,          /// Bitwise invert the value in the stencil buffer
        increment_wrap,  /// Increment the value with wrapping (255 + 1 = 0)
        decrement_wrap,  /// Decrement the value with wrapping (0 - 1 = 255)
    };

    /**
     * Specifies the blend factor used in blending calculations.
     * The source and destination factors are used to blend the source and destination colors
     * and alpha values together to produce the final result.
     */
    enum class blend_factor : uint8
    {
        zero,                /// result = 0
        one,                 /// result = 1
        src_color,           /// result = src.rgb
        one_minus_src_color, /// result = 1 - src.rgb
        dst_color,           /// result = dst.rgb
        one_minus_dst_color, /// result = 1 - dst.rgb
        src_alpha,           /// result = src.a
        one_minus_src_alpha, /// result = 1 - src.a
        dst_alpha,           /// result = dst.a
        one_minus_dst_alpha, /// result = 1 - dst.a
        // constant_color,           /// result = constant_color.rgb
        // one_minus_constant_color, /// result = 1 - constant_color.rgb
        // constant_alpha,           /// result = constant_color.a
        // one_minus_constant_alpha, /// result = 1 - constant_color.a
    };

    /**
     * Blend operation for blending source and destination colors and alpha values
     */
    enum class blend_op : uint8
    {
        add,              /// result = src + dst
        subtract,         /// result = src - dst
        reverse_subtract, /// result = dst - src
        min,              /// result = min(src, dst)
        max,              /// result = max(src, dst)
    };

    /**
     * Describes the write mask for color channels in a render target.
     * Used to select which color channels (red, green, blue, alpha) are updated
     * during rendering operations.
     */
    enum class color_mask : uint8
    {
        red = 0x1,
        green = 0x2,
        blue = 0x4,
        alpha = 0x8,
    };

    /**
     * Specifies which faces of a polygon will be culled during rendering
     * Culling is the process of discarding polygons that are not visible in the scene
     */
    enum class cull_face : uint8
    {
        off,   /// No culling; all faces are rendered
        front, /// Cull front-facing polygons
        back,  /// Cull back-facing polygons
    };

    /**
     * Specifies the winding order for front-facing polygons
     * Front-facing polygons are the ones that are visible in the scene
     */
    enum class front_face : uint8
    {
        clockwise,         /// Front faces are clockwise
        counter_clockwise, /// Front faces are counter-clockwise
    };

    /**
     * Specifies the fill mode for polygons
     */
    enum class polygon_mode : uint8
    {
        fill,   /// Normal fill (the triangle is rendered fully)
        lines,  /// Outline only (the triangle edges are rendered as lines)
        points, /// Only vertices (each vertex is rendered as a pixel/point)
    };

    /**
     * This enum defines a comprehensive list of texture formats including:
     * - Normalized formats (UNorm and SNorm)
     * - Integer formats (unsigned and signed)
     * - Floating-point formats (half and full precision)
     * - Depth and depth-stencil formats
     *
     * The enum values follow a naming convention:
     * - `r`, `rg`, `rgb`, `rgba` specify the number of color channels.
     * - Numbers (e.g., 8, 16, 32) specify the bit depth per component.
     * - Suffixes:
     *   - `un`: unsigned normalized (maps to [0, 1] in shader)
     *   - `in`: signed normalized (maps to [-1, 1] in shader)
     *   - `u`: unsigned integer
     *   - `i`: signed integer
     *   - `f`: floating-point (IEEE 754)
     *
     * Special depth and depth-stencil formats are also included.
     * These formats are used for depth testing and shadow mapping.
     */
    enum class pixel_format : uint8
    {
        none = 0,          /// No texture format

        r8un,              /// 8-bit R channel, unsigned normalized [0, 1]
        r8in,              /// 8-bit R channel, signed normalized [-1, 1]
        r16un,             /// 16-bit R channel, unsigned normalized [0, 1]
        r16in,             /// 16-bit R channel, signed normalized [-1, 1]

        rg8un,             /// 8-bit RG channels, unsigned normalized [0, 1]
        rg8in,             /// 8-bit RG channels, signed normalized [-1, 1]
        rg16un,            /// 16-bit RG channels, unsigned normalized [0, 1]
        rg16in,            /// 16-bit RG channels, signed normalized [-1, 1]

        rgb8un,            /// 8-bit RGB channels, unsigned normalized [0, 1]
        rgb8in,            /// 8-bit RGB channels, signed normalized [-1, 1]
        rgb16un,           /// 16-bit RGB channels, unsigned normalized [0, 1]
        rgb16in,           /// 16-bit RGB channels, signed normalized [-1, 1]

        rgba8un,           /// 8-bit RGBA channels, unsigned normalized [0, 1]
        rgba8in,           /// 8-bit RGBA channels, signed normalized [-1, 1]
        rgba16un,          /// 16-bit RGBA channels, unsigned normalized [0, 1]
        rgba16in,          /// 16-bit RGBA channels, signed normalized [-1, 1]

        r8u,               /// 8-bit R channel, unsigned integer [0, 255]
        r8i,               /// 8-bit R channel, signed integer [-128, 127]
        r16u,              /// 16-bit R channel, unsigned integer
        r16i,              /// 16-bit R channel, signed integer
        r32u,              /// 32-bit R channel, unsigned integer
        r32i,              /// 32-bit R channel, signed integer

        rg8u,              /// 8-bit RG channels, unsigned integer
        rg8i,              /// 8-bit RG channels, signed integer
        rg16u,             /// 16-bit RG channels, unsigned integer
        rg16i,             /// 16-bit RG channels, signed integer
        rg32u,             /// 32-bit RG channels, unsigned integer
        rg32i,             /// 32-bit RG channels, signed integer

        rgb8u,             /// 8-bit RGB channels, unsigned integer
        rgb8i,             /// 8-bit RGB channels, signed integer
        rgb16u,            /// 16-bit RGB channels, unsigned integer
        rgb16i,            /// 16-bit RGB channels, signed integer
        rgb32u,            /// 32-bit RGB channels, unsigned integer
        rgb32i,            /// 32-bit RGB channels, signed integer

        rgba8u,            /// 8-bit RGBA channels, unsigned integer
        rgba8i,            /// 8-bit RGBA channels, signed integer
        rgba16u,           /// 16-bit RGBA channels, unsigned integer
        rgba16i,           /// 16-bit RGBA channels, signed integer
        rgba32u,           /// 32-bit RGBA channels, unsigned integer
        rgba32i,           /// 32-bit RGBA channels, signed integer

        r16f,              /// 16-bit R channel, IEEE float
        r32f,              /// 32-bit R channel, IEEE float
        rg16f,             /// 16-bit RG channels, IEEE float
        rg32f,             /// 32-bit RG channels, IEEE float
        rgb16f,            /// 16-bit RGB channels, IEEE float
        rgb32f,            /// 32-bit RGB channels, IEEE float
        rgba16f,           /// 16-bit RGBA channels, IEEE float
        rgba32f,           /// 32-bit RGBA channels, IEEE float

        depth16,           /// 16-bit depth, fixed-point [0, 1]
        depth24,           /// 24-bit depth, fixed-point [0, 1]
        depth32f,          /// 32-bit depth, IEEE float
        stencil8,          /// 8-bit stencil
        depth24_stencil8,  /// Packed 24-bit depth + 8-bit stencil
        depth32f_stencil8, /// Packed 32-bit float depth + 8-bit stencil
    };

    /**
     * Specifies the fundamental type of a texture.
     * Determines dimensionality, interpretation, and certain restrictions (e.g., square size for cubemaps)
     */
    enum class texture_type : uint8
    {
        texture_2d,   /// 2D texture
        texture_3d,   /// 3D texture
        texture_cube, /// Cubemap texture
    };

    /**
     * Specifies how a texture will be used within the rendering pipeline.
     * These flags allow the renderer and GPU driver to optimize memory and access patterns accordingly.
     * Multiple flags can be combined using bitwise OR to express compound usage.
     */
    enum class texture_usage : uint8
    {
        render_target = 0x01,        /// Texture can be used as a render target (color or depth and/or stencil attachment in a framebuffer)

        sampled = 0x04,              /// Texture can be sampled in shaders (e.g., as sampler2D)

        storage = 0x08,              /// Texture can be used as a storage image (read/write access in shaders)

        transfer_source = 0x10,      /// Texture can be used as the source in copy or blit operations
        transfer_destination = 0x20, /// Texture can be used as the destination in copy or blit operations

        resolve_source = 0x40,       /// Texture can be used as the source in a multisample resolve operation (typically a MSAA render target)
        resolve_destination = 0x80,  /// Texture can be used as the destination of a resolve operation (must be non-multisampled)
    };

    /**
     * Defines the filtering method for magnification and minification.
     */
    enum class filter_mode : uint8
    {
        nearest, /// Selects the nearest texel (no filtering)
        linear,  /// Performs linear interpolation between texels
    };

    /**
     * Defines the filtering method used when selecting mipmap levels.
     */
    enum class mipmap_filter_mode : uint8
    {
        off,     /// Disables mipmap filtering; base level is always used
        nearest, /// Selects the nearest mipmap level (discrete)
        linear,  /// Performs linear interpolation between mipmap levels (trilinear filtering)
    };

    /**
     * Defines the addressing mode (wrap mode) for texture coordinates.
     */
    enum class wrap_mode : uint8
    {
        repeat,          /// Coordinates outside [0, 1] are wrapped (modulo)
        mirrored_repeat, /// Coordinates mirror every integer boundary
        clamp_to_edge,   /// Coordinates outside [0, 1] are clamped to the edge texels
        clamp_to_border, /// Coordinates outside [0, 1] return the border color
    };

    /**
     * Specifies the stage of a shader program, such as vertex or fragment.
     */
    enum class shader_stage : uint8
    {
        vertex,   /// Vertex shader
        fragment, /// Fragment shader
    };

    /**
     * Specifies the render backend API (OpenGL, Vulkan, DirectX 12, Metal, etc.)
     */
    enum class render_backend_type : uint8
    {
        opengl,    /// OpenGL backend
        vulkan,    /// Vulkan backend (not implemented yet)
        directx12, /// DirectX 12 backend (not implemented yet)
        metal,     /// Metal backend (not implemented yet)
    };

    /**
     * Describes how a framebuffer attachment is handled at the start of a render pass
     */
    enum class load_op : uint8
    {
        load,      /// Load existing contents from the attachment (preserve previous frame)
        clear,     /// Clear the attachment to a specified clear value at the start of the render pass
        dont_care, /// Attachment contents are undefined at the start; previous contents may be discarded
    };

    /**
     * Describes how a framebuffer attachment is handled at the end of a render pass
     */
    enum class store_op : uint8
    {
        store,     /// Store the rendered result to the attachment (make it available after the render pass)
        resolve,   /// Resolve the MSAA result to a single-sampled texture (only valid for multisample color attachments)
        dont_care, /// Final contents are undefined; rendering result may be discarded
    };

} // namespace tavros::renderer::rhi

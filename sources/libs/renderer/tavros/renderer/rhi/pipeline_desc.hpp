#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/flags.hpp>
#include <tavros/core/containers/static_vector.hpp>
#include <tavros/renderer/rhi/compare_op.hpp>
#include <tavros/renderer/rhi/pixel_format.hpp>
#include <tavros/renderer/rhi/vertex_attribute.hpp>
#include <tavros/renderer/rhi/limits.hpp>
#include <tavros/renderer/rhi/shader_info.hpp>

namespace tavros::renderer
{

    /**
     * Contains shader include info for the pipeline.
     */
    struct pipeline_shader
    {
        /// Shader should be compiled for this stage
        shader_stage stage = shader_stage::vertex;

        /// Source code for the shader
        core::string_view entry_point = "main";
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
     * Describes the write mask for color channels in a render target.
     * Used to select which color channels (red, green, blue, alpha) are updated
     * during rendering operations.
     */
    enum color_mask : uint8
    {
        red = 0x1,
        green = 0x2,
        blue = 0x4,
        alpha = 0x8,
    };

    constexpr core::flags<color_mask> operator|(color_mask lhs, color_mask rhs) noexcept
    {
        return core::flags<color_mask>(lhs) | core::flags<color_mask>(rhs);
    }

    constexpr core::flags<color_mask> k_default_color_mask = /// Default color mask
        color_mask::red | color_mask::green | color_mask::blue | color_mask::alpha;

    /**
     * Describes the blending properties for a single render target.
     * Controls how the source and destination colors and alpha values
     * are combined during rendering operations.
     */
    struct blend_state
    {
        /// If false, the source fully changes the destination color
        bool blend_enabled = false;

        /// Source blend factor for color channels
        blend_factor src_color_factor = blend_factor::one;

        /// Destination blend factor for color channels
        blend_factor src_alpha_factor = blend_factor::one;

        /// Blend operation for color channels
        blend_op color_blend_op = blend_op::add;

        /// Source blend factor for alpha channel
        blend_factor dst_color_factor = blend_factor::zero;

        /// Destination blend factor for alpha channel
        blend_factor dst_alpha_factor = blend_factor::zero;

        /// Blend operation for alpha channel
        blend_op alpha_blend_op = blend_op::add;

        /// Mask specifying which color channels are written
        core::flags<color_mask> mask = k_default_color_mask;
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
     * Represents the configuration and operations for stencil testing during rendering
     */
    struct stencil_state
    {
        /// Mask for reading from the stencil buffer
        uint8 read_mask = 0xFF;

        /// Mask for writing to the stencil buffer
        uint8 write_mask = 0xFF;

        /// Reference value for stencil testing
        uint8 reference_value = 0;

        /// The stencil buffer value is compared to the reference value using this function
        compare_op compare = compare_op::always;

        /// Stencil operation to perform if the stencil test fails
        stencil_op stencil_fail_op = stencil_op::keep;

        /// Stencil operation to perform if the depth test fails
        stencil_op depth_fail_op = stencil_op::keep;

        /// Stencil operation to perform if the stencil test passes
        stencil_op pass_op = stencil_op::keep;
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
     * Describes the rasterizer state for a pipeline
     * This structure defines how the GPU should rasterize and render
     * geometric primitives such as points, lines, and triangles
     */
    struct rasterizer_state
    {
        /// Controls which faces of a polygon are culled during rendering
        cull_face cull = cull_face::off;

        /// Specifies the winding order for front-facing polygons
        front_face face = front_face::counter_clockwise;

        /// Specifies the fill mode for polygons
        polygon_mode polygon = polygon_mode::fill;

        /// Enable depth clamping, which will clamp the depth value to the range [near, far]
        bool depth_clamp_enable = false;

        /// Near plane value for depth clamping
        float depth_clamp_near = 0.0f;

        /// Far plane value for depth clamping
        float depth_clamp_far = 1.0f;

        /// Enable depth biasing, which will offset the depth value by the specified amount
        bool depth_bias_enable = false;

        /// Amount of depth bias to apply (D + depth_bias + depth_bias_slope * M)
        float depth_bias = 0.0f;

        /// Slope factor for depth biasing (D + depth_bias + depth_bias_slope * M)
        float depth_bias_slope = 0.0f;

        /// Clamp value for depth biasing
        float depth_bias_clamp = 0.0f;
    };


    /**
     * Describes the multisample state (MSAA) for a pipeline
     */
    struct multisample_state
    {
        /// Number of samples per pixel should be 1 (no MSAA), 2, 4, 8, or 16
        uint32 sample_count = 1;

        /// Enables or disables sample shading (rendering with a shader for each sample)
        bool sample_shading_enabled = false;

        /// Minimum sample shading value for running sample shading. Should be between [0.0..1.0]
        float min_sample_shading = 1.0f;
    };


    /**
     * Represents the configuration for depth and stencil testing during rendering
     */
    struct depth_stencil_state
    {
        /// Enables or disables depth testing
        bool depth_test_enable = false;

        /// Enables or disables writing to the depth buffer
        bool depth_write_enable = false;

        /// Comparison function used for depth testing
        compare_op depth_compare = compare_op::off;

        /// Enables or disables stencil testing
        bool stencil_test_enable = false;

        /// Stencil state for front-facing geometry
        stencil_state stencil_front;

        /// Stencil state for back-facing geometry
        stencil_state stencil_back;
    };

    /**
     * Describes the color attachments and depth attachment for a pipeline
     */
    struct render_targets
    {
        /// Color attachment formats
        core::static_vector<pixel_format, k_max_color_attachments> color_formats;

        /// Depth attachment format
        pixel_format depth_stencil_format = pixel_format::depth24_stencil8;
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


    struct pipeline_desc
    {
        /// List with descriptions of shaders to be used in the pipeline
        core::static_vector<pipeline_shader, k_max_pipeline_shaders> shaders;

        /// List of vertex attributes
        core::static_vector<vertex_attribute, k_max_vertex_attributes> attributes;

        /// Describes the properties of a blend state for a multiple render targets
        core::static_vector<blend_state, k_max_color_attachments> blend_states;

        /// Describes the properties of depth and stencil testing
        depth_stencil_state depth_stencil;

        /// Defines how the GPU interprets and assembles vertex data into geometric primitives
        primitive_topology topology = primitive_topology::triangles;

        /// Describes how the GPU rasterizes and renders geometric primitives
        rasterizer_state rasterizer;

        /// Describes the multisample state (MSAA) for a pipeline
        multisample_state multisample;


        render_targets targets;
    };

} // namespace tavros::renderer

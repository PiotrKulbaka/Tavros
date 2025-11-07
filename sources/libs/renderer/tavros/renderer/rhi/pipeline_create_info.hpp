#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/flags.hpp>
#include <tavros/core/containers/static_vector.hpp>
#include <tavros/renderer/rhi/vertex_attribute.hpp>
#include <tavros/renderer/rhi/limits.hpp>
#include <tavros/renderer/rhi/handle.hpp>

namespace tavros::renderer::rhi
{

    constexpr core::flags<color_mask> operator|(color_mask lhs, color_mask rhs) noexcept
    {
        return core::flags<color_mask>(lhs) | core::flags<color_mask>(rhs);
    }

    constexpr core::flags<color_mask> k_rgba_color_mask = /// Default color mask
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
        blend_factor src_color_factor = blend_factor::src_alpha;

        /// Destination blend factor for color channels
        blend_factor dst_color_factor = blend_factor::one_minus_src_alpha;

        /// Blend operation for color channels
        blend_op color_blend_op = blend_op::add;

        /// Source blend factor for alpha channel
        blend_factor src_alpha_factor = blend_factor::one;

        /// Destination blend factor for alpha channel
        blend_factor dst_alpha_factor = blend_factor::one_minus_src_alpha;

        /// Blend operation for alpha channel
        blend_op alpha_blend_op = blend_op::add;

        /// Mask specifying which color channels are written
        core::flags<color_mask> mask = k_rgba_color_mask;
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

        /// Amount of depth bias to apply (D + depth_bias + depth_bias_factor * M)
        float depth_bias = 0.0f;

        /// Slope factor for depth biasing (D + depth_bias + depth_bias_factor * M)
        float depth_bias_factor = 0.0f;

        /// Clamp value for depth biasing
        float depth_bias_clamp = 0.0f;

        /// Enable scissor for the pipeline
        bool scissor_enable = false;
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

    struct pipeline_create_info
    {
        /// List with descriptions of shaders to be used in the pipeline
        core::static_vector<shader_handle, k_max_pipeline_shaders> shaders;

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

} // namespace tavros::renderer::rhi

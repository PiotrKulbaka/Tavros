#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/flags.hpp>
#include <tavros/core/containers/fixed_vector.hpp>
#include <tavros/renderer/rhi/limits.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/enums.hpp>

namespace tavros::renderer::rhi
{

    /**
     * @brief Provides information for buffer base binding.
     */
    struct bind_buffer_info
    {
        /// Vertex buffer handle
        buffer_handle buffer;

        /// Offset in bytes from the start of the GPU buffer to the beginning of the vertex data
        uint32 base_offset = 0;
    };

    /**
     * @brief Describes a texture + sampler binding to a shader stage.
     */
    struct texture_binding
    {
        /// Handle to the texture resource to bind
        texture_handle texture;

        /// Handle to the sampler resource to bind
        sampler_handle sampler;

        /// Binding slot in the shader (matches `layout(binding=X)` in GLSL/HLSL)
        uint32 binding = 0;
    };

    /**
     * @brief Describes a binding of a buffer or subrange of a buffer to a shader stage.
     */
    struct buffer_binding
    {
        /// Handle to the buffer resource to bind
        buffer_handle buffer;

        /// Byte offset from the start of the buffer (used for dynamic or subrange bindings)
        uint32 offset = 0;

        /// Size of the bound range in bytes (0 means the entire buffer is bound)
        uint32 size = 0;

        /// Binding slot in the shader (matches `layout(binding=X)`)
        uint32 binding = 0;
    };

    /**
     * @brief Defines the region of the framebuffer used for rasterization.
     */
    struct viewport_info
    {
        /// Lower-left corner X position, in pixels.
        int32 left = 0;

        /// Lower-left corner Y position, in pixels.
        int32 top = 0;

        /// Width of the viewport, in pixels.
        int32 width = 0;

        /// Height of the viewport, in pixels.
        int32 height = 0;
    };

    /**
     * @brief Defines a rectangular scissor region
     * Only pixels inside this rectangle are written to the render target.
     */
    struct scissor_info
    {
        /// Left corner X position, in pixels.
        int32 left = 0;

        /// Lower corner Y position, in pixels.
        int32 top = 0;

        /// Scissor rectangle width, in pixels.
        int32 width = 0;

        /// Scissor rectangle height, in pixels.
        int32 height = 0;
    };

    /**
     * @brief Describes a region of a texture to copy to or from a buffer.
     * This structure is used when performing partial texture updates or readbacks.
     */
    struct texture_copy_region
    {
        /// Offset in bytes within the buffer where the copy starts
        size_t buffer_offset = 0;

        /// Row length in texels within the buffer.
        /// If 0, rows are assumed to be tightly packed (row length = width of the region).
        uint32 buffer_row_length = 0;

        /// Mipmap level of the texture to copy.
        uint32 mip_level = 0;

        /// Index of the layer (or cube face) to copy.
        uint32 layer_index = 0;

        /// X offset in texels within the texture where the copy region starts.
        uint32 x_offset = 0;

        /// Y offset in texels within the texture where the copy region starts.
        uint32 y_offset = 0;

        /// Z offset in texels within the texture where the copy region starts.
        /// For 2D textures, should be 0. For 3D textures, specifies the starting depth slice.
        uint32 z_offset = 0;

        /// Width of the region to copy, in texels.
        uint32 width = 0;

        /// Height of the region to copy, in texels.
        uint32 height = 0;

        /// Depth of the region to copy, in texels.
        /// For 2D textures or cube faces, should be 1. For 3D textures, specifies number of slices.
        uint32 depth = 1;
    };


    // ------------------------------------------------------------------------
    // Shader reflection
    // ------------------------------------------------------------------------

    /**
     * @brief Reflection data for a vertex attribute.
     * Describes a single input attribute of a vertex shader.
     */
    struct vertex_attribute_reflect
    {
        /// Attribute name in shader
        core::fixed_string<63> name;

        /// Value shape (only: scalar, vec2, vec3, vec4)
        composite_format format = composite_format::scalar;

        /// Base scalar type of attribute
        scalar_type type = scalar_type::f32;

        /// Number of elements (0 = not array)
        uint32 array_size = 0;

        /// Attribute location index
        uint32 location = 0;
    };

    /**
     * @brief Reflection data for a shader resource binding.
     * Represents samplers, images or buffers used by the shader.
     */
    struct shader_resource_reflect
    {
        /// Resource name in shader.
        core::fixed_string<63> name;

        /// Type of shader resource.
        shader_resource_type type = shader_resource_type::sampler_2d;

        /// Binding point.
        uint32 binding = 0;
    };

    /**
     * @brief Reflection data for a single member inside a constant block.
     * Used for introspection of constant buffer layouts.
     */
    struct member_reflect
    {
        /// Member name.
        core::fixed_string<63> name;

        /// Value shape (scalar, vector or matrix).
        composite_format format = composite_format::scalar;

        /// Base scalar type.
        scalar_type type = scalar_type::f32;

        /// Byte offset from the beginning of the parent block.
        uint32 offset = 0;

        /// Number of elements in the array (0 = not an array).
        uint32 array_size = 0;

        /// Byte stride between array elements.
        /// Valid only when @ref array_size is greater than zero.
        uint32 array_stride = 0;

        /// Byte stride between matrix columns or rows.
        /// Valid only for matrix types (format == matX).
        uint32 matrix_stride = 0;

        /// Matrix storage order.
        /// @c true for row-major matrices, @c false for column-major matrices.
        /// Valid only for matrix types (format == matX).
        bool is_row_major = false;
    };

    /**
     * @brief Reflection data for a constant buffer (UBO).
     */
    struct constant_block_reflect
    {
        /// Block name
        core::fixed_string<63> name;

        /// Total size in bytes
        uint32 size = 0;

        /// Binding point
        uint32 binding = 0;
    };

    /**
     * @brief Reflection data for a storage buffer (SSBO).
     */
    struct storage_block_reflect
    {
        /// Block name
        core::fixed_string<63> name;

        /// Total size in bytes
        uint32 size = 0;

        /// Binding point
        uint32 binding = 0;
    };

    /**
     * @brief Reflection data for a shader output variable.
     */
    struct output_reflect
    {
        /// Output variable name
        core::fixed_string<63> name;

        /// Output shape (only: scalar, vec2, vec3, vec4)
        composite_format format = composite_format::scalar;

        /// Output scalar type
        scalar_type type = scalar_type::f32;

        /// Output location index
        uint32 location = 0;
    };

    /**
     * @brief Compute shader reflection.
     */
    struct compute_reflect
    {
        /// Is compute shader.
        bool is_compute = false;

        /// Number of invocations in X dimension.
        uint32 local_size_x = 0;

        /// Number of invocations in Y dimension
        uint32 local_size_y = 0;

        /// Number of invocations in Z dimension.
        uint32 local_size_z = 0;
    };


    // ------------------------------------------------------------------------
    // Texture create info
    // ------------------------------------------------------------------------

    constexpr core::flags<texture_usage> operator|(texture_usage lhs, texture_usage rhs) noexcept
    {
        return core::flags<texture_usage>(lhs) | core::flags<texture_usage>(rhs);
    }

    constexpr core::flags<texture_usage> k_default_texture_usage = /// Default texture usage
        texture_usage::sampled | texture_usage::transfer_destination;

    /**
     * Describes properties of a texture to be created by the renderer.
     * This includes pixel format, dimensions, usage, mipmaps, array layers, and multisampling.
     * This struct is passed to the backend texture creation function.
     */
    struct texture_create_info
    {
        /// Type of the texture
        texture_type type = texture_type::texture_2d;

        /// Pixel format defining color channels, bit depth, and data layout
        pixel_format format = pixel_format::rgba8un;

        /// Texture width in pixels. Must be > 0
        uint32 width = 0;

        /// Texture height in pixels. Must be > 0
        uint32 height = 0;

        /// Texture depth (for 3D textures) Must be > 0, for texture_2d and texture_cube must be 1
        uint32 depth = 1;

        /// Bit flags describing allowed usage patterns of this texture (e.g., sampled, render target)
        /// Used by renderer to optimize memory and access
        core::flags<texture_usage> usage = k_default_texture_usage;

        /// Number of mipmap levels, 1 indicates no mipmaps
        uint32 mip_levels = 1;

        ///  Number of array layers (for texture arrays). 1 means a single texture, must be >= 1
        uint32 array_layers = 1;

        /// Number of samples per pixel for multisampling (MSAA) (1 = no MSAA). Must be a power of two where supported
        uint32 sample_count = 1;
    };


    // ------------------------------------------------------------------------
    // Sampler create info
    // ------------------------------------------------------------------------

    /**
     * Describes how the sampler filters texels and mipmaps
     */
    struct sampler_filter
    {
        /// Filter used when texture is minified
        filter_mode min_filter = filter_mode::nearest;

        /// Filter used when texture is magnified
        filter_mode mag_filter = filter_mode::nearest;

        /// Filter used to select between mipmap levels
        mipmap_filter_mode mipmap_filter = mipmap_filter_mode::off;
    };

    /**
     * Describes the wrapping behavior in each texture dimension.
     */
    struct sampler_wrap_mode
    {
        /// Wrapping mode for U (S) axis
        wrap_mode wrap_s = wrap_mode::repeat;

        /// Wrapping mode for V (T) axis
        wrap_mode wrap_t = wrap_mode::repeat;

        /// Wrapping mode for W (R) axis (for 3D textures)
        wrap_mode wrap_r = wrap_mode::repeat;
    };

    /**
     * Describes a full sampler configuration for use in shaders.
     * This structure is passed to the sampler creation function in the rendering backend.
     */
    struct sampler_create_info
    {
        /// Filtering modes (min, mag, mipmap)
        sampler_filter filter;

        /// Wrapping modes for texture coordinates
        sampler_wrap_mode wrap_mode;

        /// Bias added to the computed LOD value (used to sharpen or blur textures)
        float mip_lod_bias = 0.0f;

        /// Minimum LOD that can be selected by the sampler
        float min_lod = 0.0f;

        /// Maximum LOD that can be selected by the sampler
        float max_lod = 1000.0f;

        /// Comparison function used when sampling depth textures
        compare_op depth_compare = compare_op::off;
    };


    // ------------------------------------------------------------------------
    // Shader create info
    // ------------------------------------------------------------------------

    /**
     * @brief Provides a complete set of shader program sources.
     */
    struct shader_create_info
    {
        /// Vertex shader source.
        core::string_view vertex_shader_source;

        /// Fragment shader source.
        core::string_view fragment_shader_source;
    };


    // ------------------------------------------------------------------------
    // Buffer create info
    // ------------------------------------------------------------------------

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


    // ------------------------------------------------------------------------
    // Framebuffer create info
    // ------------------------------------------------------------------------

    /**
     * Describes a single color attachment used in a framebuffer
     */
    struct color_attachment_info
    {
        /// Handle to the color attachment texture.
        texture_handle target = {};

        /// Resolve target texture used when @ref store is set to @c store_op::resolve.
        texture_handle resolve_target = {};

        /// Load operation performed at the beginning of the render pass
        load_op load = load_op::dont_care;

        /// Store operation performed at the end of the render pass
        store_op store = store_op::dont_care;

        /// Clear color value used when @ref load is set to @c load_op::clear.
        float clear_value[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    /**
     * Describes a depth/stencil attachment used in a framebuffer
     * Note: depth and stencil operations can be controlled independently if needed
     */
    struct depth_stencil_attachment_info
    {
        /// Handle to the depth-stencil attachment texture
        texture_handle target = {};

        /// Resolve target texture used when @ref depth_store or @ref stencil_store is set to @c store_op::resolve.
        texture_handle resolve_target = {};

        /// Load operation for the depth component
        load_op depth_load = load_op::dont_care;

        /// Store operation for the depth component
        store_op depth_store = store_op::dont_care;

        /// Clear value for the depth component (used only if depth_load == clear)
        float depth_clear_value = 1.0f;

        /// Load operation for the stencil component
        load_op stencil_load = load_op::dont_care;

        /// Store operation for the stencil component.
        store_op stencil_store = store_op::dont_care;

        /// Clear depth value used when @ref depth_load is set to @c load_op::clear.
        int32 stencil_clear_value = 0;
    };

    /**
     * Describes a complete framebuffer configuration.
     */
    struct framebuffer_create_info
    {
        /// Framebuffer width in pixels. Must match the width of all attachments.
        uint32 width = 0;

        /// Framebuffer height in pixels. Must match the height of all attachments.
        uint32 height = 0;

        /// List of color attachments. Each attachment's format must match the pipeline layout.
        core::fixed_vector<color_attachment_info, k_max_color_attachments> color_attachments;

        /// Depth/stencil attachment description.
        /// Disabled when attachment handle is invalid.
        depth_stencil_attachment_info depth_stencil_attachment;

        /// Number of samples per pixel. Typical values are 1, 2, 4, 8 or 16.
        /// All attachments must use the same sample count.
        uint32 sample_count = 1;
    };


    // ------------------------------------------------------------------------
    // Frame composer create info
    // ------------------------------------------------------------------------

    /**
     * Describes a swapchain to be created for a window or view.
     */
    struct frame_composer_create_info
    {
        /// Width of the rendering surface, in pixels
        uint32 width = 0;

        /// Height of the rendering surface, in pixels
        uint32 height = 0;

        /// Whether vertical synchronization (v-sync) is enabled
        bool vsync = false;

        /// Description of the color attachment used for rendering to the swapchain
        pixel_format color_attachment_format = pixel_format::none;

        /// Description of the optional depth/stencil attachment used during rendering (format may be none)
        pixel_format depth_stencil_attachment_format = pixel_format::none;

        /// Number of back buffers to create (usually 2 for double-buffering or 3 for triple-buffering)
        uint32 buffer_count = 3;

        /// Swap interval
        int32 swap_interval = -1; // FPS limit, -1 - default; 0 - no limit

        /// Native handle associated with the frame composer (e.g., platform-specific window or surface)
        void* native_handle = nullptr;
    };


    // ------------------------------------------------------------------------
    // Pipeline create info
    // ------------------------------------------------------------------------

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
     * Defines a single vertex attribute within a vertex buffer layout
     * Specifies how one attribute (e.g., position, color, normal) is stored and interpreted
     */
    struct vertex_attribute
    {
        /// Format of attribute
        composite_format format = composite_format::scalar;

        /// Type of each component
        scalar_type type = scalar_type::f32;

        /// Whether integer data should be normalized to [0,1] or [-1,1] (only applicable for integer types)
        bool normalize = false;

        /// Attribute location in the shader
        uint32 location = 0;

        /// Stride in bytes between consecutive vertices in the buffer
        uint32 stride = 0;

        /// Offset in bytes from the start of the vertex to this attribute (0 for densely packed)
        uint32 offset = 0;

        /// Specifies how often this attribute advances per instance when rendering with instancing
        /// - 0: attribute is a per-vertex value (changes every vertex)
        /// - 1: attribute is a per-instance value (changes once per instance)
        /// - N (>1): attribute is reused for N consecutive instances before advancing
        uint32 instance_divisor = 0;
    };

    /**
     * Describes the configuration for a color attachment in a render pass.
     */
    struct color_attachment_state
    {
        /// Format of the attachment
        pixel_format format = pixel_format::none;

        /// Mask specifying which color channels are written
        core::flags<color_mask> mask = k_rgba_color_mask;

        /// Blending configuration
        blend_state blend;
    };

    /**
     * Represents the configuration for depth and stencil testing during rendering
     */
    struct depth_stencil_state
    {
        /// Depth stencil attachment format
        pixel_format format = pixel_format::none;

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
     * Describes a pipeline.
     */
    struct pipeline_create_info
    {
        /// Shader program to be used in the pipeline
        shader_handle shader_program;

        /// Array of attribute bindings describing how vertex attributes are read from buffers
        core::fixed_vector<vertex_attribute, k_max_vertex_attributes> bindings;

        /// Describes the properties of a blend state for a multiple render targets
        core::fixed_vector<color_attachment_state, k_max_color_attachments> color_attachments;

        /// Describes the properties of depth and stencil testing
        depth_stencil_state depth_stencil_attachment;

        /// Defines how the GPU interprets and assembles vertex data into geometric primitives
        primitive_topology topology = primitive_topology::triangles;

        /// Describes how the GPU rasterizes and renders geometric primitives
        rasterizer_state rasterizer;

        /// Describes the multisample state (MSAA) for a pipeline
        multisample_state multisample;
    };

} // namespace tavros::renderer::rhi

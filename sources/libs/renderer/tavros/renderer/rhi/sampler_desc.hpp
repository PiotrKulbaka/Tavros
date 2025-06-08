#pragma once

#include <tavros/core/types.hpp>
#include <tavros/renderer/rhi/compare_op.hpp>

namespace tavros::renderer
{

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
    struct sampler_desc
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

} // namespace tavros::renderer

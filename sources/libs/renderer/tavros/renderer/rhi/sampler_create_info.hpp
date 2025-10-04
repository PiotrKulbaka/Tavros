#pragma once

#include <tavros/renderer/rhi/enums.hpp>

namespace tavros::renderer::rhi
{

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

} // namespace tavros::renderer::rhi

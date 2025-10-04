#pragma once

#include <tavros/core/flags.hpp>
#include <tavros/renderer/rhi/enums.hpp>

namespace tavros::renderer::rhi
{

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

} // namespace tavros::renderer::rhi

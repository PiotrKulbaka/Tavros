#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/flags.hpp>
#include <tavros/renderer/rhi/pixel_format.hpp>

namespace tavros::renderer
{

    /**
     * Specifies how a texture will be used within the rendering pipeline.
     * These flags allow the renderer and GPU driver to optimize memory and access patterns accordingly.
     * Multiple flags can be combined using bitwise OR to express compound usage.
     */
    enum class texture_usage : uint8
    {
        render_target = 0x01,        /// Texture can be used as a render target (color attachment in a framebuffer)
        depth_stencil_target = 0x02, /// Texture can be used as a depth and/or stencil attachment

        sampled = 0x04,              /// Texture can be sampled in shaders (e.g., as sampler2D)

        storage = 0x08,              /// Texture can be used as a storage image (read/write access in shaders)

        transfer_source = 0x10,      /// Texture can be used as the source in copy or blit operations
        transfer_destination = 0x20, /// Texture can be used as the destination in copy or blit operations

        resolve_source = 0x40,       /// Texture can be used as the source in a multisample resolve operation (typically a MSAA render target)
        resolve_destination = 0x80,  /// Texture can be used as the destination of a resolve operation (must be non-multisampled)
    };

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
    struct texture_desc
    {
        /// Pixel format defining color channels, bit depth, and data layout
        pixel_format format = pixel_format::rgba8un;

        /// Texture width in pixels. Must be > 0
        uint32 width = 0;

        /// Texture height in pixels. Must be > 0
        uint32 height = 0;

        /// Texture depth (for 3D textures). Must be > 1 for 3D, else 1
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

} // namespace tavros::renderer

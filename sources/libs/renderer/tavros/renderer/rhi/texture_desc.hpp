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
        shader_resource = 0x1, /// Texture will be used as a shader resource (sampled in shaders)
        render_target = 0x2,   /// Texture can be used as a render target (output of rendering)
        depth_stencil = 0x4,   /// Texture can be used as a depth and/or stencil buffer
        transfer_src = 0x8,    /// Texture can be used as source for copy operations
        transfer_dst = 0x10,   /// Texture can be used as destination for copy operations
        cpu_write = 0x20,      /// Texture memory can be updated/written from the CPU
    };

    constexpr core::flags<texture_usage> operator|(texture_usage lhs, texture_usage rhs) noexcept
    {
        return core::flags<texture_usage>(lhs) | core::flags<texture_usage>(rhs);
    }

    constexpr core::flags<texture_usage> k_default_texture_usage = /// Default texture usage
        texture_usage::shader_resource | texture_usage::transfer_dst | texture_usage::cpu_write;

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

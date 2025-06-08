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
     * Describes the properties of a 2D texture to be created by the renderer.
     * Includes pixel format, dimensions, mipmapping, array layers, sample count for MSAA, usage flags, and optional initial data.
     * This structure is passed to the texture creation function in the rendering backend.
     */
    struct texture2d_desc
    {
        /// Format of texture pixels (color channels and bit depth)
        pixel_format format = pixel_format::rgba8un;

        /// Width of the texture in pixels
        uint32 width = 0;

        /// Height of the texture in pixels
        uint32 height = 0;

        /// Number of mipmap levels (1 means no mipmaps)
        uint32 mip_levels = 1;

        /// Number of array layers (1 for regular 2D texture, >1 for texture arrays)
        uint32 array_layers = 1;

        /// Number of samples per pixel for multisampling (1 = no MSAA)
        uint32 sample_count = 1;

        /// Flags specifying intended usage scenarios of the texture
        core::flags<texture_usage> usage = k_default_texture_usage;

        /// Pointer to initial pixel data (can be nullptr if no initial data)
        void* data = nullptr;

        /// Number of bytes per row (scanline) in the data buffer (can be 0 for auto-compute)
        uint32 stride = 0;
    };

} // namespace tavros::renderer

#pragma once


#include <tavros/core/string_view.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/texture_create_info.hpp>


namespace tavros::renderer
{

    struct texture_info
    {
        rhi::texture_handle h;
        rhi::texture_type   type;
        rhi::pixel_format   format;
        uint32              width;
        uint32              height;
        uint32              depth;
    };

    /*
    texture_type type = texture_type::texture_2d; /// Type of the texture
    pixel_format format = pixel_format::rgba8un; /// Pixel format defining color channels, bit depth, and data layout
    uint32 width = 0; /// Texture width in pixels. Must be > 0
    uint32 height = 0; /// Texture height in pixels. Must be > 0
    uint32 depth = 1; /// Texture depth (for 3D textures) Must be > 0, for texture_2d and texture_cube must be 1
    core::flags<texture_usage> usage = k_default_texture_usage; /// Bit flags describing allowed usage patterns of this texture (e.g., sampled, render target)
    uint32 mip_levels = 1; /// Number of mipmap levels, 1 indicates no mipmaps
    uint32 array_layers = 1; ///  Number of array layers (for texture arrays). 1 means a single texture, must be >= 1
    uint32 sample_count = 1; /// Number of samples per pixel for multisampling (MSAA) (1 = no MSAA). Must be a power of two where supported
    */

    class texture_manager
    {
    public:
        rhi::texture_handle load(core::string_view name, bool gen_mipmaps = true, rhi::texture_type tt = rhi::texture_type::texture_2d, rhi::pixel_format pf = rhi::pixel_format::none);

        // void rhi::texture_handle retain()
    };

} // namespace tavros::renderer

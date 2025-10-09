#pragma once

#include <tavros/core/object_view.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/geometry_create_info.hpp>

namespace tavros::renderer
{

    struct mesh_create_info
    {
        /// Type of the texture
        rhi::texture_type type = rhi::texture_type::texture_2d;

        /// Pixel format defining color channels, bit depth, and data layout
        rhi::pixel_format format = rhi::pixel_format::rgba8un;

        /// Texture width in pixels. Must be > 0
        uint32 width = 0;

        /// Texture height in pixels. Must be > 0
        uint32 height = 0;

        /// Texture depth (for 3D textures) Must be > 0, for texture_2d and texture_cube must be 1
        uint32 depth = 1;

        /// Number of mipmap levels, 1 indicates no mipmaps
        uint32 mip_levels = 1;

        ///  Number of array layers (for texture arrays). 1 means a single texture, must be >= 1
        uint32 array_layers = 1;
    };

    class mesh
    {
    public:
        ~mesh();

        bool has_index_buffer() const noexcept;

        rhi::index_buffer_format index_format() const noexcept;

        rhi::buffer_handle vertex_buffer_handle() const noexcept;

        rhi::buffer_handle index_buffer_handle() const noexcept;
    };

} // namespace tavros::renderer


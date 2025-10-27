#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer::rhi
{

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

} // namespace tavros::renderer::rhi

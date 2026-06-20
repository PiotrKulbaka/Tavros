#pragma once

#include <tavros/core/resource/resource.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/enums.hpp>

namespace tavros::renderer
{

    struct texture_t : public core::basic_resource<texture_t>
    {
        /// Texture dimensionality type
        rhi::texture_type type = rhi::texture_type::texture_2d;

        /// Pixel format on the GPU
        rhi::pixel_format format = rhi::pixel_format::none;

        /// Width of a single tile/face in pixels
        uint32 width = 0;

        /// Height of a single tile/face in pixels
        uint32 height = 0;

        /// Depth in pixels (texture_3d only)
        uint32 depth = 1;

        /// Number of array layers (texture_2d only)
        uint32 array_layers = 1;

        /// Native GPU texture handle
        rhi::texture_handle gpu_texture;
    };

    using texture_ref = core::basic_resource_ref<texture_t>;

} // namespace tavros::renderer

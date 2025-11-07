#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer::rhi
{

    /**
     * @brief Defines the region of the framebuffer used for rasterization.
     */
    struct viewport_info
    {
        /// Lower-left corner X position, in pixels.
        int32 left = 0;

        /// Lower-left corner Y position, in pixels.
        int32 top = 0;

        /// Width of the viewport, in pixels.
        int32 width = 0;

        /// Height of the viewport, in pixels.
        int32 height = 0;
    };

    /**
     * @brief Defines a rectangular scissor region
     * Only pixels inside this rectangle are written to the render target.
     */
    struct scissor_info
    {
        /// Left corner X position, in pixels.
        int32 left = 0;

        /// Lower corner Y position, in pixels.
        int32 top = 0;

        /// Scissor rectangle width, in pixels.
        int32 width = 0;

        /// Scissor rectangle height, in pixels.
        int32 height = 0;
    };

} // namespace tavros::renderer::rhi

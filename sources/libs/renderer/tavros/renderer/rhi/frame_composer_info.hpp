#pragma once

#include <tavros/core/types.hpp>
#include <tavros/renderer/rhi/pixel_format.hpp>

namespace tavros::renderer::rhi
{

    /**
     * Describes a swapchain to be created for a window or view.
     */
    struct frame_composer_info
    {
        /// Width of the rendering surface, in pixels
        uint32 width = 0;

        /// Height of the rendering surface, in pixels
        uint32 height = 0;

        /// Whether vertical synchronization (v-sync) is enabled
        bool vsync = false;

        /// Description of the color attachment used for rendering to the swapchain
        pixel_format color_attachment_format;

        /// Description of the optional depth/stencil attachment used during rendering (format may be none)
        pixel_format depth_stencil_attachment_format;

        /// Number of back buffers to create (usually 2 for double-buffering or 3 for triple-buffering)
        uint32 buffer_count = 3;
    };

} // namespace tavros::renderer::rhi

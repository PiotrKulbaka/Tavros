#pragma once

#include <tavros/core/types.hpp>
#include <tavros/renderer/rhi/attachment_info.hpp>

namespace tavros::renderer
{

    /**
     * Describes a swapchain to be created for a window or view.
     */
    struct swapchain_desc
    {
        /// Width of the rendering surface, in pixels
        uint32 width = 0;

        /// Height of the rendering surface, in pixels
        uint32 height = 0;

        /// Whether vertical synchronization (v-sync) is enabled
        bool vsync = false;

        /// Description of the color attachment used for rendering to the swapchain
        color_attachment_info color_attachment;

        /// Description of the optional depth/stencil attachment used during rendering (format may be none)
        depth_stencil_attachment_info depth_stencil_attachment;

        /// Number of back buffers to create (usually 2 for double-buffering or 3 for triple-buffering)
        uint32 buffer_count = 3;
    };

} // namespace tavros::renderer

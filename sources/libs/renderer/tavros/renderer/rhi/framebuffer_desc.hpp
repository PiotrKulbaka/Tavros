#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/renderer/rhi/attachment_info.hpp>
#include <tavros/renderer/rhi/limits.hpp>

namespace tavros::renderer
{

    /**
     * Describes a complete framebuffer configuration
     */
    struct framebuffer_desc
    {
        /// List of color attachments. Must match the pipeline layout
        core::static_vector<color_attachment_info, k_max_color_attachments> color_attachments;

        /// Optional depth/stencil attachment. Must match the pipeline layout
        depth_stencil_attachment_info depth_stencil_attachment;

        /// Framebuffer width, in pixels. Must match all attachments
        uint32 width = 0;

        /// Framebuffer height, in pixels. Must match all attachments
        uint32 height = 0;

        /// Number of samples per pixel. Must be 1 (no MSAA), 2, 4, 8, or 16
        /// All attachments must use the same sample count.
        uint32 sample_count = 1;
    };

} // namespace tavros::renderer

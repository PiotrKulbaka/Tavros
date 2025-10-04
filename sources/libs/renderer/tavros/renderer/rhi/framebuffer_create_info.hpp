#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/renderer/rhi/enums.hpp>
#include <tavros/renderer/rhi/limits.hpp>

namespace tavros::renderer::rhi
{

    /**
     * Describes a complete framebuffer configuration
     */
    struct framebuffer_create_info
    {
        /// Framebuffer width, in pixels. Must match all attachments
        uint32 width = 0;

        /// Framebuffer height, in pixels. Must match all attachments
        uint32 height = 0;

        /// List of color attachments. Must match the pipeline layout
        core::static_vector<pixel_format, k_max_color_attachments> color_attachment_formats;

        /// Optional depth/stencil attachment. Must match the pipeline layout
        pixel_format depth_stencil_attachment_format = pixel_format::none;

        /// Number of samples per pixel. Must be 1 (no MSAA), 2, 4, 8, or 16
        /// All attachments must use the same sample count. (Except resolve attachments)
        uint32 sample_count = 1;
    };

} // namespace tavros::renderer::rhi

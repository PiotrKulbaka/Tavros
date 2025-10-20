#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/renderer/rhi/enums.hpp>
#include <tavros/renderer/rhi/limits.hpp>
#include <tavros/renderer/rhi/handle.hpp>

namespace tavros::renderer::rhi
{

    /**
     * Describes a complete framebuffer configuration
     */
    struct framebuffer_create_info
    {
        /// Framebuffer width in pixels. Must match the width of all attachments
        uint32 width = 0;

        /// Framebuffer height in pixels. Must match the height of all attachments
        uint32 height = 0;

        /// List of color attachments. Each attachment's format must match the pipeline layout
        core::static_vector<texture_handle, k_max_color_attachments> color_attachments;

        /// Whether the framebuffer has a depth-stencil attachment
        bool has_depth_stencil_attachment = false;

        /// Handle to the depth-stencil attachment texture
        texture_handle depth_stencil_attachment;

        /// Number of samples per pixel (MSAA). Must be 1, 2, 4, 8, 16 ... etc
        /// All attachments must use the same sample count
        uint32 sample_count = 1;
    };

} // namespace tavros::renderer::rhi

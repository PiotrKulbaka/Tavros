#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/renderer/rhi/pixel_format.hpp>
#include <tavros/renderer/rhi/limits.hpp>

namespace tavros::renderer
{

    /**
     * Describes how a framebuffer attachment is handled at the start of a render pass
     */
    enum class load_op : uint8
    {
        load,      /// Load existing contents from the attachment (preserve previous frame)
        clear,     /// Clear the attachment to a specified clear value at the start of the render pass
        dont_care, /// Attachment contents are undefined at the start; previous contents may be discarded
    };

    /**
     * Describes how a framebuffer attachment is handled at the end of a render pass
     */
    enum class store_op : uint8
    {
        store,     /// Store the rendered result to the attachment (make it available after the render pass)
        resolve,   /// Resolve the MSAA result to a single-sampled texture (only valid for multisample color attachments)
        dont_care, /// Final contents are undefined; rendering result may be discarded
    };

    /**
     * Describes a single color attachment used in a framebuffer
     */
    struct color_attachment_info
    {
        /// Format of the attachment. Must be a color format
        pixel_format format = pixel_format::none;

        /// Number of samples per pixel should be 1 (no MSAA), 2, 4, 8, or 16. Must match the framebuffer sample count
        uint32 sample_count = 1;

        /// Load operation performed at the beginning of the render pass
        load_op load = load_op::dont_care;

        /// Store operation performed at the end of the render pass
        store_op store = store_op::dont_care;

        // Index of the resolve target attachment, used when store_op is set to `resolve`
        uint32 resolve_texture_index = 0;

        /// Clear color value used when load_op is set to `clear`
        float clear_value[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    /**
     * Describes a depth/stencil attachment used in a framebuffer
     * Note: depth and stencil operations can be controlled independently if needed
     */
    struct depth_stencil_attachment_info
    {
        /// Format of the attachment. Must be a depth, stencil, or depth-stencil format or none
        pixel_format format = pixel_format::none;

        /// Load operation for the depth component
        load_op depth_load = load_op::dont_care;

        /// Store operation for the depth component
        store_op depth_store = store_op::dont_care;

        /// Clear value for the depth component (used only if depth_load == clear)
        float depth_clear_value = 1.0f;

        /// Load operation for the stencil component
        load_op stencil_load = load_op::dont_care;

        /// Store operation for the stencil component
        store_op stencil_store = store_op::dont_care;

        /// Clear value for the stencil component (used only if stencil_load == clear)
        int32 stencil_clear_value = 0;
    };

    /**
     * Describes a complete render pass configuration
     */
    struct render_pass_info
    {
        /// List of color attachments. The order matches the layout in the framebuffer
        core::static_vector<color_attachment_info, k_max_color_attachments> color_attachments;

        /// Optional depth/stencil attachment
        depth_stencil_attachment_info depth_stencil_attachment;
    };

} // namespace tavros::renderer

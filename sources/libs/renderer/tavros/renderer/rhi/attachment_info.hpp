#pragma once

#include <tavros/renderer/rhi/pixel_format.hpp>

namespace tavros::renderer
{

    /**
     * Describes how a framebuffer attachment is handled at the start of a render pass
     */
    enum class load_op : uint8
    {
        load,      /// Load existing contents from the attachment
        clear,     /// Clear the attachment to a specified clear value
        dont_care, /// Attachment contents are undefined
    };

    /**
     * Describes how a framebuffer attachment is handled at the end of a render pass
     */
    enum class store_op : uint8
    {
        store,     /// Store the result to the attachment
        dont_care, /// Result is discarded
    };

    /**
     * Describes a single color attachment used in a framebuffer
     */
    struct color_attachment_info
    {
        /// Format of the attachment. Must be a color format
        pixel_format format = pixel_format::none;

        /// Load operation performed before rendering begins
        load_op load = load_op::dont_care;

        /// Store operation performed after rendering ends
        store_op store = store_op::dont_care;

        /// Clear color used when load_op is set to clear
        float clear_value[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    /**
     * Describes a depth/stencil attachment used in a framebuffer
     */
    struct depth_stencil_attachment_info
    {
        /// Format of the attachment. Must be a depth/stencil format or none
        pixel_format format = pixel_format::none;

        /// Load operation performed before rendering begins
        load_op load = load_op::dont_care;

        /// Store operation performed after rendering ends
        store_op store = store_op::dont_care;

        /// Clear value for the depth component, if supported
        float depth_clear_value = 1.0f;

        /// Clear value for the stencil component, if supported
        uint32 stencil_clear_value = 0;
    };

} // namespace tavros::renderer

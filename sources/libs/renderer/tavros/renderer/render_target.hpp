#pragma once

#include <tavros/core/containers/static_vector.hpp>
#include <tavros/core/object/object_view.hpp>
#include <tavros/core/memory/buffer_view.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/limits.hpp>
#include <tavros/renderer/rhi/texture_create_info.hpp>

namespace tavros::renderer
{

    struct render_target_create_info
    {
        /// All texture attachments will have this format
        rhi::texture_type target_type = rhi::texture_type::texture_2d;

        /// Width of the render target
        uint32 width = 0;

        /// Height of the render target
        uint32 height = 0;

        /// Depth of the render target
        uint32 depth = 1;

        /// List of color attachments
        core::static_vector<rhi::pixel_format, rhi::k_max_color_attachments> color_attachment_formats;

        /// Depth/stencil attachment
        rhi::pixel_format depth_stencil_attachment_format = rhi::pixel_format::none;

        /// Number of samples per pixel. Must be 1 (no MSAA), 2, 4, 8, or 16
        uint32 sample_count = 1;
    };

    class render_target
    {
    public:
        ~render_target() = default;

        [[nodiscard]] const render_target_create_info& info() const noexcept
        {
            return m_info;
        }

        [[nodiscard]] bool has_color_attachments() const noexcept
        {
            return m_color_attachments.size() != 0;
        }

        [[nodiscard]] bool has_depth_stencil_attachment() const noexcept
        {
            return m_depth_stencil_attachment.valid();
        }

        [[nodiscard]] uint32 color_attachment_count() const noexcept
        {
            return static_cast<uint32>(m_color_attachments.size());
        }

        [[nodiscard]] rhi::texture_handle color_attachment(uint32 index) const noexcept
        {
            TAV_ASSERT(index < color_attachment_count());
            return m_color_attachments[index];
        }

        [[nodiscard]] rhi::texture_handle depth_stencil_attachment() const noexcept
        {
			TAV_ASSERT(has_depth_stencil_attachment());
            return m_depth_stencil_attachment;
        }

        rhi::framebuffer_handle framebuffer() const
        {
            return m_framebuffer;
        }

    private:
        render_target(
            rhi::framebuffer_handle                framebuffer,
            const render_target_create_info&       info,
            core::buffer_view<rhi::texture_handle> coror_attachments,
            rhi::texture_handle                    depth_stencil_attachment
        ) noexcept
            : m_framebuffer(framebuffer)
            , m_info(info)
            , m_color_attachments(coror_attachments)
            , m_depth_stencil_attachment(depth_stencil_attachment)
        {
        }

        friend class core::object_pool<render_target>; // for empalce_add in render_system

    private:
        using textures_vector = core::static_vector<rhi::texture_handle, rhi::k_max_color_attachments>;
        rhi::framebuffer_handle   m_framebuffer;
        render_target_create_info m_info;
        textures_vector           m_color_attachments;
        rhi::texture_handle       m_depth_stencil_attachment;
    };

    using render_target_view = core::object_view<render_target>;

} // namespace tavros::renderer

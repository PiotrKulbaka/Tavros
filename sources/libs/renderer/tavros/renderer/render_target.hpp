#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

namespace tavros::renderer
{

    class render_target : core::noncopyable
    {
    public:
        render_target(core::buffer_view<rhi::pixel_format> color_attachment_formats, rhi::pixel_format depth_stencil_attachment_format) noexcept;

        ~render_target();

        void init(rhi::graphics_device* gdevice);

        void shutdown();

        void recreate(uint32 width, uint32 height, uint32 msaa);

        uint32 color_attachment_count() const;

        rhi::texture_handle get_color_attachment(uint32 index) const;

        rhi::texture_handle get_depth_stencil_attachment() const;

        rhi::framebuffer_handle framebuffer() const;

        rhi::render_pass_handle render_pass() const;

    private:
        void destroy_all();

        rhi::framebuffer_handle create_fb(uint32 width, uint32 height, uint32 msaa);

        rhi::texture_handle create_texture(uint32 width, uint32 height, rhi::pixel_format fmt, tavros::core::flags<rhi::texture_usage> usage, uint32 msaa);

        rhi::render_pass_handle create_rp(bool need_resolve);

    private:
        template<class T>
        using vector_t = tavros::core::static_vector<T, rhi::k_max_color_attachments>;

        bool                          m_is_created;
        rhi::graphics_device*         m_gdevice;
        vector_t<rhi::pixel_format>   m_color_attachment_formats;
        rhi::pixel_format             m_depth_stencil_attachment_format;
        vector_t<rhi::texture_handle> m_resolve_source_color_attachments;
        vector_t<rhi::texture_handle> m_resolve_destination_color_attachments;
        rhi::texture_handle           m_resolve_source_depth_stencil_attachment;
        rhi::texture_handle           m_resolve_destination_depth_stencil_attachment;
        rhi::framebuffer_handle       m_framebuffer;
        rhi::render_pass_handle       m_render_pass;
    };


} // namespace tavros::renderer

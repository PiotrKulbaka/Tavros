#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/resource/resource.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/render_target/render_target_desc.hpp>

namespace tavros::renderer
{

    /**
     * @brief Offscreen render target with optional MSAA resolve.
     *
     * Manages a set of color attachments and a depth/stencil attachment,
     * along with the corresponding framebuffer and render pass.
     *
     * When MSAA is enabled (sample count > 1), each attachment has a
     * multisample source texture and a resolved single-sample destination
     * texture. Getters always return the resolved (destination) textures,
     * which are suitable for sampling in subsequent passes.
     *
     * Typical usage:
     * @code
     * rhi::pixel_format formats[] = { rhi::pixel_format::rgba8 };
     * render_target rt(formats, rhi::pixel_format::depth24_stencil8);
     * rt.resize(1920, 1080, 4);
     *
     * // render loop
     * cmd.begin_rendering(rt.framebuffer());
     * // ...
     * cmd.end_rendering();
     *
     * // use result
     * auto color = rt.color_attachment(0);
     * @endcode
     *
     * @note Not copyable. Movable.
     */
    class render_target : public core::basic_resource<render_target>, core::noncopyable
    {
    public:
        /** @brief Default constructor. */
        render_target(rhi::graphics_device* gdevice, const render_target_desc& desc) noexcept;

        /** @brief Move constructor. */
        render_target(render_target&& other) noexcept;

        /** @brief Destroys the render target, releasing all GPU resources. */
        ~render_target() noexcept;

        /**
         * @brief Allocates or reallocates GPU resources at the given resolution and sample count.
         *
         * Destroys any previously created resources before recreating them.
         * On failure, all partially created resources are cleaned up automatically.
         *
         * @param width   Render target width in pixels. Must be > 0.
         * @param height  Render target height in pixels. Must be > 0.
         * @param msaa    Sample count. Pass 1 to disable MSAA.
         */
        void resize(uint32 width, uint32 height, uint32 msaa = 1);

        /**
         * @brief Returns the number of color attachments.
         *
         * Equals the size of the color_attachment_formats array passed to the constructor.
         */
        uint32 attachment_count() const;

        /**
         * @brief Returns the resolved color attachment texture with the given name.
         */
        rhi::texture_handle color_attachment_by_name(core::string_view name) const noexcept;

        /**
         * @brief Returns the resolved color attachments.
         *
         * When MSAA is enabled, returns the single-sample resolved textures, suitable for
         * sampling. When MSAA is disabled, returns the render texture directly.
         *
         * @pre  m_is_created == true
         */
        core::buffer_view<rhi::texture_handle> color_attachments() const noexcept;

        /**
         * @brief Returns the resolved depth/stencil attachment texture.
         *
         * When MSAA is enabled, returns the single-sample resolved texture.
         * When MSAA is disabled, returns the render texture directly.
         *
         * @pre  m_is_created == true
         */
        rhi::texture_handle depth_stencil_attachment() const;

        /**
         * @brief Returns the framebuffer handle.
         *
         * The framebuffer references the multisample source textures (when MSAA is enabled).
         * Pass to begin_rendering().
         *
         * @pre  m_is_created == true
         */
        rhi::framebuffer_handle gpu_framebuffer() const;

    private:
        void destroy_all();

        rhi::framebuffer_handle create_fb(uint32 width, uint32 height);

        rhi::texture_handle create_texture(uint32 width, uint32 height, rhi::pixel_format fmt, tavros::core::flags<rhi::texture_usage> usage, uint32 msaa);

    private:
        using color_attachment_config = render_target_desc::color_attachment_config;
        using depth_attachment_config = render_target_desc::depth_attachment_config;
        using stencil_attachment_config = render_target_desc::stencil_attachment_config;

        template<class T>
        using vector_t = core::fixed_vector<T, rhi::k_max_color_attachments>;

        uint32 m_current_msaa;

        rhi::graphics_device* m_gdevice;            // Non-owning pointer to the graphics

        vector_t<color_attachment_config> m_ca_cfg; // Color attachments config
        depth_attachment_config           m_da_cfg; // Depth attachment config
        stencil_attachment_config         m_sa_cfg; // Stencil attachment config

        vector_t<rhi::texture_handle> m_src_cl;     // Source color attachment
        vector_t<rhi::texture_handle> m_dst_cl;     // Destination color attachment
        rhi::texture_handle           m_src_ds;     // Source depth stencil attachment
        rhi::texture_handle           m_dst_ds;     // Destination depth stencil attachment

        rhi::framebuffer_handle m_framebuffer;      // Framebuffer referencing src attachments
    };

    using render_target_ref = core::basic_resource_ref<render_target>;

} // namespace tavros::renderer

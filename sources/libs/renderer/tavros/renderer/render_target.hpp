#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

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
     * rt.init(device);
     * rt.resize(1920, 1080, 4);
     *
     * // render loop
     * cmd.begin_render_pass(rt.render_pass(), rt.framebuffer());
     * // ...
     * cmd.end_render_pass();
     *
     * // use result
     * auto color = rt.color_attachment(0);
     * @endcode
     *
     * @note Not copyable. Movable.
     * @note init() must be called before resize() or any getters.
     */
    class render_target : core::noncopyable
    {
    public:
        /** @brief Default constructor. */
        render_target() noexcept;

        /**
         * @brief Move constructor.
         */
        render_target(render_target&& other) noexcept;

        /**
         * @brief Destroys the render target, releasing all GPU resources.
         *
         * Calls shutdown() internally. Safe to call even if init() was never called.
         */
        ~render_target() noexcept;

        /**
         * @brief Move assignment.
         */
        render_target& operator=(render_target&& other) noexcept;

        /**
         * @brief Binds a graphics device to this render target.
         *
         * Must be called exactly once before resize().
         *
         * @param gdevice  Non-owning pointer to the graphics device.
         *                 Must outlive this render target.
         * @param color_attachment_formats  Pixel formats for each color attachment.
         *                                  Must not contain pixel_format::none.
         * @param depth_stencil_attachment_format  Pixel format for the depth/stencil attachment.
         *                                         Pass pixel_format::none to omit it.
         * @pre  gdevice != nullptr
         * @pre  init() has not been called before on this instance
         */
        void init(rhi::graphics_device* gdevice, core::buffer_view<rhi::pixel_format> color_attachment_formats, rhi::pixel_format depth_stencil_attachment_format);

        /**
         * @brief Releases all GPU resources created by resize().
         *
         * Safe to call multiple times. Does nothing if no resources are allocated.
         */
        void shutdown();

        /**
         * @brief Allocates or reallocates GPU resources at the given resolution and sample count.
         *
         * Destroys any previously created resources before recreating them.
         * On failure, all partially created resources are cleaned up automatically.
         *
         * @param width   Render target width in pixels. Must be > 0.
         * @param height  Render target height in pixels. Must be > 0.
         * @param msaa    Sample count. Pass 1 to disable MSAA.
         * @pre  init() must have been called before resize()
         */
        void resize(uint32 width, uint32 height, uint32 msaa = 0);

        /**
         * @brief Returns the number of color attachments.
         *
         * Equals the size of the color_attachment_formats array passed to the constructor.
         */
        uint32 attachment_count() const;

        /**
         * @brief Returns the resolved color attachment texture at the given index.
         *
         * When MSAA is enabled, returns the single-sample resolved texture,
         * suitable for sampling. When MSAA is disabled, returns the render texture directly.
         *
         * @param index  Attachment index. Must be < attachment_count().
         * @pre  m_is_created == true
         */
        rhi::texture_handle color_attachment(uint32 index) const;

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
         * Pass to begin_render_pass() together with render_pass().
         *
         * @pre  m_is_created == true
         */
        rhi::framebuffer_handle framebuffer() const;

        /**
         * @brief Returns the render pass handle.
         *
         * Encodes load/store/resolve operations for all attachments.
         * Compatible with the framebuffer returned by framebuffer().
         *
         * @pre  m_is_created == true
         */
        rhi::render_pass_handle render_pass() const;

    private:
        void destroy_all();

        rhi::framebuffer_handle create_fb(uint32 width, uint32 height, uint32 msaa);

        rhi::texture_handle create_texture(uint32 width, uint32 height, rhi::pixel_format fmt, tavros::core::flags<rhi::texture_usage> usage, uint32 msaa);

        rhi::render_pass_handle create_rp(bool need_resolve);

    private:
        template<class T>
        using vector_t = core::fixed_vector<T, rhi::k_max_color_attachments>;

        bool m_is_init;
        bool m_is_created;
        bool m_is_msaa_enabled;

        rhi::graphics_device* m_gdevice;        // Non-owning pointer to the graphics

        vector_t<rhi::pixel_format> m_cl_fmt;   // Color ttachment formats
        rhi::pixel_format           m_ds_fmt;   // Depth stencil color ttachment format

        vector_t<rhi::texture_handle> m_src_cl; // Source color attachment
        vector_t<rhi::texture_handle> m_dst_cl; // Destination color attachment
        rhi::texture_handle           m_src_ds; // Source depth stencil attachment
        rhi::texture_handle           m_dst_ds; // Destination depth stencil attachment

        rhi::framebuffer_handle m_framebuffer;  // Framebuffer referencing src attachments
        rhi::render_pass_handle m_render_pass;  // Render pass with load/store/resolve ops
    };


} // namespace tavros::renderer

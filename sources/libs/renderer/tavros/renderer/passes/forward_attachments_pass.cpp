#include <tavros/renderer/passes/forward_attachments_pass.hpp>

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/renderer/render_system.hpp>

namespace
{
    tavros::core::logger logger("forward_attachments_pass");
}

namespace tavros::renderer
{

    forward_attachments_pass::forward_attachments_pass(render_system* rs)
        : m_rs(rs)
    {
        m_rt = m_rs->resource_manager()->load_render_target("render_targets.forward_render_target");
        if (!m_rt.is_ready()) {
            logger.error("Failed to load render target 'render_targets.forward_render_target'");
        }
        TAV_ASSERT(m_rt);
    }

    forward_attachments_pass::~forward_attachments_pass() noexcept
    {
        m_rs->resource_manager()->release_render_target(m_rt);
        m_rt = {};
        m_rs = nullptr;
    }

    void forward_attachments_pass::upload(frame_context& ctx)
    {
        const auto& frame_info = ctx.frame_data().get_data<fd_frame_info>();
        const auto& graphics = ctx.frame_data().get_data<fd_graphics_settings>();

        const auto width = static_cast<uint32>(frame_info.frame_size.width);
        const auto height = static_cast<uint32>(frame_info.frame_size.height);
        const auto msaa = graphics.msaa;

        if (width == 0 || height == 0 || msaa == 0) {
            logger.error("Invalid frame size: {}x{}, or msaa: {}", width, height, msaa);
            return;
        }

        if (width != m_current_width || height != m_current_height || msaa != m_current_msaa) {
            m_current_width = width;
            m_current_height = height;
            m_current_msaa = msaa;
            m_rt->resize(width, height, msaa);
        }

        auto base_color = m_rt->color_attachment_by_name("base_color");
        auto depth_stencil = m_rt->depth_stencil_attachment();
        ctx.frame_data().add_data<fd_base_color_ca>(base_color);
        ctx.frame_data().add_data<fd_depth_stencil_ca>(depth_stencil);
    }

    void forward_attachments_pass::execute(frame_context& ctx)
    {
        ctx.commands().begin_rendering(m_rt->gpu_framebuffer());
    }

} // namespace tavros::renderer

#include <tavros/renderer/renderer.hpp>

#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("renderer");
}

namespace tavros::renderer
{

    /*renderer::renderer(void* native_window_handle)
    {
        // First of all create the graphics device
        m_device = rhi::graphics_device::create(rhi::rhi_backend::opengl);
        if (!m_device) {
            ::logger.error("Failed to create renderer: graphics device not created");
            TAV_DEBUG_BREAK();
            return;
        }

        // Next create the frame composer
        rhi::frame_composer_info composer_info;
        composer_info.width = 0;
        composer_info.height = 0;
        composer_info.vsync = false;
        composer_info.color_attachment_format;
        composer_info.depth_stencil_attachment_format;
        composer_info.buffer_count = 3;

        m_composer_h = m_device->create_frame_composer(composer_info, native_window_handle);
    }

    renderer::~renderer()
    {
    }*/

} // namespace tavros::renderer
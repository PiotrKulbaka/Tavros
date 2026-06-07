#include <tavros/renderer/render_system.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("render_system");
}

namespace tavros::renderer
{

    render_system::render_system() noexcept
    {
    }

    render_system::~render_system() noexcept
    {
    }

    void render_system::init(void* main_window_native_handle)
    {
        if (m_initialized) {
            logger.warning("Render system is already initialized.");
            return;
        }

        m_graphics_device = rhi::graphics_device::create(rhi::render_backend_type::opengl);
        if (!m_graphics_device) {
            logger.error("Failed to create graphics device.");
            return;
        }

        rhi::frame_composer_create_info fc_info;
        fc_info.width = 1;
        fc_info.height = 1;
        fc_info.buffer_count = 2;
        fc_info.vsync = false;
        fc_info.color_attachment_format = rhi::pixel_format::rgba8un;
        fc_info.depth_stencil_attachment_format = rhi::pixel_format::depth24_stencil8;
        fc_info.native_handle = main_window_native_handle;

        auto fc_handle = m_graphics_device->create_frame_composer(fc_info);
        if (!fc_handle) {
            release();
            logger.error("Failed to create frame composer.");
            return;
        }

        m_composer = m_graphics_device->get_frame_composer_ptr(fc_handle);
        TAV_ASSERT(m_composer);

        m_initialized = true;
    }

    void render_system::shutdown() noexcept
    {
        if (!m_initialized) {
            logger.warning("Render system is not initialized.");
            return;
        }

        release();

        m_initialized = false;
    }

    void render_system::release() noexcept
    {
        m_graphics_device = nullptr;
    }

} // namespace tavros::renderer
#include <tavros/renderer/render_system.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("render_system");
}

namespace tavros::renderer
{

    render_system::render_system(core::shared_ptr<assets::asset_manager> am, core::shared_ptr<tef::workspace> ws) noexcept
        : m_am(am)
        , m_ws(ws)
    {
    }

    render_system::~render_system() noexcept
    {
        if (m_is_init) {
            logger.warning("shutdown() was not called before the render_system was destroyed");
            shutdown();
        }
    }

    void render_system::init(void* main_window_native_handle)
    {
        if (m_is_init) {
            logger.warning("Render system is already initialized.");
            return;
        }

        m_gdevice = rhi::graphics_device::create(rhi::render_backend_type::opengl);
        if (!m_gdevice) {
            logger.error("Failed to create graphics device.");
            return;
        }

        rhi::frame_composer_create_info fc_info;
        fc_info.width = 1;
        fc_info.height = 1;
        fc_info.buffer_count = 2;
        fc_info.vsync = false;
        fc_info.color_attachment_format = rhi::pixel_format::rgba8un;
        fc_info.depth_stencil_attachment_format = rhi::pixel_format::none;
        fc_info.native_handle = main_window_native_handle;

        auto fc_handle = m_gdevice->create_frame_composer(fc_info);
        if (!fc_handle) {
            logger.error("Failed to create frame composer.");
            return;
        }

        m_composer = m_gdevice->get_frame_composer_ptr(fc_handle);
        TAV_ASSERT(m_composer);

        m_rm = core::make_unique<tavros::renderer::resource_manager>(m_gdevice.get(), m_am, m_ws);
        if (!m_rm) {
            logger.error("Failed to create resource manager.");
            return;
        }

        m_is_init = true;
    }

    void render_system::shutdown() noexcept
    {
        if (!m_is_init) {
            logger.warning("Render system is not initialized.");
        }

        m_am = nullptr;
        m_ws = nullptr;
        m_rm = nullptr;
        m_gdevice = nullptr;

        m_is_init = false;
    }

    void render_system::begin_frame() noexcept
    {
        if (!m_is_init) {
            logger.error("Render system is not initialized.");
            return;
        }

        m_rm->begin_frame();
    }

    void render_system::end_frame() noexcept
    {
        m_rm->end_frame();
        ++m_frame_number;
        m_composer->present();
    }

    void render_system::resize_backbuffer(uint32 width, uint32 height)
    {
        m_composer->resize(width, height);
    }

    rhi::framebuffer_handle render_system::gpu_backbuffer() noexcept
    {
        return m_composer->backbuffer();
    }

} // namespace tavros::renderer
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

        m_upctx = core::make_unique<upload_context>(m_gdevice.get());
        if (!m_upctx) {
            logger.error("Failed to create upload context.");
            return;
        }

        m_rm = core::make_unique<tavros::renderer::resource_manager>(m_gdevice.get(), m_am, m_ws);
        if (!m_rm) {
            logger.error("Failed to create resource manager.");
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

        auto fc_handle = m_gdevice->create_frame_composer(fc_info);
        if (!fc_handle) {
            logger.error("Failed to create frame composer.");
            return;
        }

        m_composer = m_gdevice->get_frame_composer_ptr(fc_handle);
        TAV_ASSERT(m_composer);

        auto* upload_cmd = m_composer->create_command_queue();
        m_upctx->begin_frame(upload_cmd);

        m_rm->init(m_upctx.get());

        m_is_init = true;
    }

    void render_system::shutdown() noexcept
    {
        if (!m_is_init) {
            logger.warning("Render system is not initialized.");
        }

        m_am = nullptr;
        m_ws = nullptr;
        m_rm->shutdown();
        m_upctx = nullptr;
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

        if (m_frame_number == 0) {
            m_upctx->end_frame(); // First frame started with init
        }

        auto* upload_cmd = m_composer->create_command_queue();
        m_upctx->begin_frame(upload_cmd);
        m_rm->begin_frame();
    }

    void render_system::end_frame() noexcept
    {
        m_rm->end_frame();
        m_upctx->end_frame();
        ++m_frame_number;
    }

} // namespace tavros::renderer
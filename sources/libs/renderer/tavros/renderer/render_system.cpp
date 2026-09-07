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

        m_rm = core::make_unique<renderer::resource_manager>(m_gdevice.get(), m_am, m_ws);
        if (!m_rm) {
            logger.error("Failed to create resource manager.");
            return;
        }

        m_ui_r = core::make_unique<renderer::ui_renderer>(m_gdevice.get(), m_rm.get());
        if (!m_ui_r) {
            logger.error("Failed to create 2D renderer.");
            return;
        }

        m_constant_buffer.init(m_gdevice.get(), 256 * 1024 * 1024, rhi::buffer_usage::constant, "render_system.constant_buffer");

        m_is_init = true;
    }

    void render_system::shutdown() noexcept
    {
        if (!m_is_init) {
            logger.warning("Render system is not initialized.");
        }

        m_ui_r = nullptr;
        m_am = nullptr;
        m_ws = nullptr;
        m_rm = nullptr;
        m_constant_buffer.shutdown();
        m_gdevice = nullptr;

        m_is_init = false;
    }

    void render_system::set_camera(const camera& cam) noexcept
    {
        m_current_view_data.set_camera_data(cam);
    }

    frame_context render_system::begin_frame(uint64 delta_time_us) noexcept
    {
        if (!m_is_init) {
            logger.error("Render system is not initialized.");
            return frame_context();
        }

        m_total_time_us += delta_time_us;
        m_delta_time_us = delta_time_us;
        const auto total_time = static_cast<float>(m_total_time_us) / 1'000'000.0f;
        const auto delta_time = static_cast<float>(m_delta_time_us) / 1'000'000.0f;
        m_current_view_data.set_frame_info(
            {static_cast<float>(m_frame_width),
             static_cast<float>(m_frame_height)},
            total_time, delta_time,
            m_frame_number
        );

        m_constant_buffer.reset();
        auto current_view_binding = m_current_view_data.upload(m_constant_buffer);

        m_rm->begin_frame();
        m_ui_r->begin_frame();

        m_current_command_queue = m_gdevice->create_command_queue();
        m_current_command_queue->bind_shader_buffers(current_view_binding);

        return frame_context(m_current_command_queue, &m_constant_buffer);
    }

    void render_system::end_frame() noexcept
    {
        TAV_ASSERT(m_is_init);
        if (m_current_command_queue) {
            m_gdevice->submit_command_queue(m_current_command_queue);
            m_current_command_queue = nullptr;
        }
        m_ui_r->end_frame();
        m_rm->end_frame();
        ++m_frame_number;
        m_composer->present();
    }

    void render_system::resize_backbuffer(uint32 width, uint32 height)
    {
        m_frame_width = width < 1 ? 1 : width;
        m_frame_height = height < 1 ? 1 : height;
        m_composer->resize(m_frame_width, m_frame_height);
    }

    rhi::framebuffer_handle render_system::gpu_backbuffer() noexcept
    {
        return m_composer->backbuffer();
    }

} // namespace tavros::renderer
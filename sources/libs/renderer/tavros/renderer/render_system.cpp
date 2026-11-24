#include <tavros/renderer/render_system.hpp>

#include <tavros/renderer/passes/camera_pass.hpp>
#include <tavros/renderer/passes/ui_render_pass.hpp>
#include <tavros/renderer/passes/present_frame_pass.hpp>
#include <tavros/renderer/passes/forward_attachments_pass.hpp>
#include <tavros/renderer/passes/debug_render_pass.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("render_system");
}

namespace tavros::renderer
{

    render_system::render_system(core::shared_ptr<assets::asset_manager> am, core::shared_ptr<tef::workspace> ws, void* window_handle)
        : m_am(am)
        , m_ws(ws)
        , m_frame_graph(this)
        , m_current_frame_context()
    {
        m_gd = rhi::graphics_device::create(rhi::render_backend_type::opengl);
        if (!m_gd) {
            logger.error("Failed to create graphics device.");
            throw std::runtime_error("Failed to create graphics device.");
        }

        rhi::frame_composer_create_info fc_info;
        fc_info.width = 1;
        fc_info.height = 1;
        fc_info.buffer_count = 2;
        fc_info.vsync = false;
        fc_info.color_attachment_format = rhi::pixel_format::rgba8un;
        fc_info.depth_stencil_attachment_format = rhi::pixel_format::none;
        fc_info.native_handle = window_handle;

        auto fc_handle = m_gd->create_frame_composer(fc_info);
        if (!fc_handle) {
            logger.error("Failed to create frame composer.");
            return;
        }

        m_composer = m_gd->get_frame_composer_ptr(fc_handle);
        TAV_ASSERT(m_composer);

        m_rm = core::make_unique<renderer::resource_manager>(m_gd.get(), m_am, m_ws);
        if (!m_rm) {
            logger.error("Failed to create resource manager.");
            return;
        }

        m_stream_buffer.init(m_gd.get(), 256 * 1024 * 1024, rhi::buffer_usage::storage, rhi::buffer_access::cpu_to_gpu, "render_system.storage_buffer");
        m_constant_buffer.init(m_gd.get(), 16 * 1024 * 1024, rhi::buffer_usage::constant, rhi::buffer_access::cpu_to_gpu, "render_system.constant_buffer");
        m_vertex_stream_buffer.init(m_gd.get(), 256 * 1024 * 1024, rhi::buffer_usage::vertex, rhi::buffer_access::cpu_to_gpu, "render_system.vertex_stream_buffer");
        m_index_stream_buffer.init(m_gd.get(), 256 * 1024 * 1024, rhi::buffer_usage::index, rhi::buffer_access::cpu_to_gpu, "render_system.index_stream_buffer");

        create_graph();
        m_pipeline = std::move(m_frame_graph.compile_for({"PresentFramePass"}));
        if (!m_pipeline) {
            logger.error("Failed to create execution rendering pipeline.");
            return;
        }
    }

    render_system::~render_system() noexcept
    {
        m_pipeline = nullptr;
        m_am = nullptr;
        m_ws = nullptr;
        m_rm = nullptr;
        m_stream_buffer.shutdown();
        m_constant_buffer.shutdown();
        m_vertex_stream_buffer.shutdown();
        m_index_stream_buffer.shutdown();
        m_gd = nullptr;
    }

    void render_system::resize_backbuffer(uint32 width, uint32 height)
    {
        m_frame_width = width < 1 ? 1 : width;
        m_frame_height = height < 1 ? 1 : height;
        m_composer->resize(m_frame_width, m_frame_height);
    }

    frame_context& render_system::begin_frame(uint64 delta_time_us)
    {
        m_total_time_us += delta_time_us;
        m_delta_time_us = delta_time_us;
        const auto frame_size = math::vec2(static_cast<float>(m_frame_width), static_cast<float>(m_frame_height));
        const auto total_time = static_cast<float>(m_total_time_us) / 1'000'000.0f;
        const auto delta_time = static_cast<float>(m_delta_time_us) / 1'000'000.0f;

        m_rm->begin_frame();

        m_current_frame_context = frame_context(m_gd.get(), m_rm.get(), &m_stream_buffer, &m_constant_buffer, &m_vertex_stream_buffer, &m_index_stream_buffer, m_frame_number);
        auto& fd = m_current_frame_context.frame_data();

        fd.add_data<fd_backbuffer>(m_composer->backbuffer());
        fd.add_data<fd_frame_info>(frame_size, total_time, delta_time);

        return m_current_frame_context;
    }

    void render_system::end_frame()
    {
        m_pipeline->run(m_current_frame_context);

        m_rm->end_frame();

        if (m_current_frame_context.m_cq) {
            m_gd->submit_command_queue(m_current_frame_context.m_cq);
            m_current_frame_context.clean();
        }

        ++m_frame_number;
        m_composer->present();
        m_stream_buffer.reset();
        m_constant_buffer.reset();
        m_vertex_stream_buffer.reset();
        m_index_stream_buffer.reset();
    }

    void render_system::create_graph()
    {
        m_frame_graph.register_pass<camera_pass>("CameraPass");

        m_frame_graph.register_pass<forward_attachments_pass>("ForwardAttachmentsPass")
            .depends_on(core::buffer_view<const core::string_view>({"CameraPass"}));

        m_frame_graph.register_pass<debug_render_pass>("DebugRenderPass")
            .depends_on(core::buffer_view<const core::string_view>({"CameraPass", "ForwardAttachmentsPass"}));

        m_frame_graph.register_pass<ui_render_pass>("UIRenderPass")
            .depends_on(core::buffer_view<const core::string_view>({"CameraPass", "ForwardAttachmentsPass"}));

        m_frame_graph.register_pass<present_frame_pass>("PresentFramePass")
            .depends_on(core::buffer_view<const core::string_view>({"DebugRenderPass", "UIRenderPass"}));
    }

} // namespace tavros::renderer
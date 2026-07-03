#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>

#include <tavros/renderer/shader_bindings/render_view.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/resource_manager.hpp>
#include <tavros/renderer/renderer2d.hpp>

namespace tavros::renderer
{

    class render_system;

    class frame_context
    {
    public:
        rhi::command_queue& commands() noexcept
        {
            return *m_cmd;
        }

        gpu_stream_buffer& constant_buffer() noexcept
        {
            return *m_constant;
        }

    private:
        friend class render_system;

        frame_context() noexcept = default;

        frame_context(rhi::command_queue* cmd, gpu_stream_buffer* constant) noexcept
            : m_cmd(cmd)
            , m_constant(constant)
        {
            TAV_ASSERT(m_cmd);
            TAV_ASSERT(m_constant);
        }

    private:
        rhi::command_queue* m_cmd = nullptr;
        gpu_stream_buffer*  m_constant = nullptr;
    };


    class render_system final : core::noncopyable
    {
    public:
        render_system(core::shared_ptr<assets::asset_manager> am, core::shared_ptr<tef::workspace> ws) noexcept;
        ~render_system() noexcept;

        void init(void* main_window_native_handle);
        void shutdown() noexcept;

        void set_camera(const camera& cam) noexcept;

        frame_context begin_frame(uint64 delta_time_us) noexcept;

        void end_frame() noexcept;

        void resize_backbuffer(uint32 width, uint32 height);

        rhi::framebuffer_handle gpu_backbuffer() noexcept;

        // TODO: This is a temporary accessor. Remove it.
        // The render system should eventually provide higher-level APIs for resource
        // creation and management, and the graphics device should be encapsulated.
        rhi::graphics_device* get_graphics_device() noexcept
        {
            return m_gdevice.get();
        }

        resource_manager* resource_manager() noexcept
        {
            return m_rm.get();
        }

        renderer2d* renderer2d() noexcept
        {
            return m_renderer2d.get();
        }

    private:
        uint64                                  m_frame_number = 0;
        uint64                                  m_delta_time_us = 0;
        uint64                                  m_total_time_us = 0;
        uint32                                  m_frame_width = 1;
        uint32                                  m_frame_height = 1;
        bool                                    m_is_init = false;
        core::shared_ptr<assets::asset_manager> m_am;
        core::shared_ptr<tef::workspace>        m_ws;
        core::unique_ptr<rhi::graphics_device>  m_gdevice;

        core::unique_ptr<renderer::resource_manager> m_rm;
        core::unique_ptr<renderer::renderer2d>       m_renderer2d;

        rhi::frame_composer* m_composer = nullptr;

        rhi::command_queue* m_current_command_queue = nullptr;

        gpu_stream_buffer m_constant_buffer;

        render_view m_current_view_data;
    };

} // namespace tavros::renderer

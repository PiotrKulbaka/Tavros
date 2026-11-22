#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/resource_manager.hpp>

#include <tavros/renderer/frame/frame_graph.hpp>

namespace tavros::renderer
{

    class render_system final : core::noncopyable, core::nonmovable
    {
    public:
        render_system(core::shared_ptr<assets::asset_manager> am, core::shared_ptr<tef::workspace> ws, void* window_handle) noexcept;
        ~render_system() noexcept;

        void resize_backbuffer(uint32 width, uint32 height);

        template<class T>
        bool add_data()
        {
            return m_current_frame_context.frame_data().add_data<T>();
        }

        template<class T>
        bool add_data(T&& data)
        {
            return m_current_frame_context.frame_data().add_data<T>(std::move(data));
        }

        frame_context& begin_frame(uint64 delta_time_us);

        void end_frame();

        rhi::graphics_device* get_graphics_device() noexcept
        {
            return m_gd.get();
        }

        resource_manager* resource_manager() noexcept
        {
            return m_rm.get();
        }


    private:
        void create_graph();

    private:
        uint64 m_frame_number = 0;
        uint64 m_delta_time_us = 0;
        uint64 m_total_time_us = 0;
        uint32 m_frame_width = 1;
        uint32 m_frame_height = 1;


        core::unique_ptr<rhi::graphics_device>  m_gd;
        core::shared_ptr<assets::asset_manager> m_am;
        core::shared_ptr<tef::workspace>        m_ws;
        gpu_stream_buffer                       m_stream_buffer;
        gpu_stream_buffer                       m_constant_buffer;

        core::unique_ptr<renderer::resource_manager> m_rm;

        rhi::frame_composer* m_composer = nullptr;

        frame_context m_current_frame_context;

        frame_graph                       m_frame_graph;
        core::unique_ptr<render_pipeline> m_pipeline;
    };

} // namespace tavros::renderer

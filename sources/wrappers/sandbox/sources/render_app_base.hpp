#pragma once

#include "event_queue.hpp"
#include <tavros/system/interfaces/application.hpp>
#include <atomic>
#include <thread>

namespace app
{

    class render_app_base : tavros::core::noncopyable
    {
    public:
        render_app_base(tavros::core::string_view name);
        virtual ~render_app_base();

        void* native_window_handle() const noexcept;

        void                set_location(int32 left, int32 top);
        tavros::math::ivec2 location() const;

        void                set_client_size(int32 width, int32 height);
        tavros::math::ivec2 client_size() const;

        bool is_closed() const;

        // should be called in the main thread
        void run();

        virtual void init() = 0;

        virtual void shutdown() = 0;

        virtual void render(event_queue_view events, double delta_time) = 0;

    private:
        void init_window_callbacks();

        void render_thread_main();

        void start_render_thread();

        void stop_render_thread();

    private:
        tavros::core::unique_ptr<tavros::system::interfaces::window> m_wnd;

        event_queue m_event_queue;

        std::atomic<bool> m_running;
        std::thread       m_render_thread;
        bool              m_is_closed = false;
    };

} // namespace app

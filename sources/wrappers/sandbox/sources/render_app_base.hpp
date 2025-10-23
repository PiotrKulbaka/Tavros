#pragma once

#include "event_queue.hpp"
#include <tavros/system/window.hpp>
#include <atomic>
#include <thread>

namespace app
{

    class render_app_base : public tavros::system::window
    {
    public:
        render_app_base(tavros::core::string_view title);
        virtual ~render_app_base() override;

        bool is_closed() const;

        // should be called in the main thread
        void run();

        virtual void init() = 0;

        virtual void shutdown() = 0;

        virtual void render(event_queue_view events, double delta_time) = 0;

    private:
        virtual void on_close(tavros::system::close_event_args& e) override;
        virtual void on_activate() override;
        virtual void on_deactivate() override;
        // virtual void on_drop(tavros::system::drop_event_args& e) override;
        // virtual void on_move(tavros::system::move_event_args& e) override;
        virtual void on_resize(tavros::system::size_event_args& e) override;
        virtual void on_mouse_down(tavros::system::mouse_event_args& e) override;
        virtual void on_mouse_move(tavros::system::mouse_event_args& e) override;
        virtual void on_mouse_up(tavros::system::mouse_event_args& e) override;
        // virtual void on_mouse_wheel(tavros::system::mouse_event_args& e) override;
        virtual void on_key_down(tavros::system::key_event_args& e) override;
        virtual void on_key_up(tavros::system::key_event_args& e) override;
        // virtual void on_key_press(tavros::system::key_event_args& e) override;

        void render_thread_main();

        void start_render_thread();

        void stop_render_thread();

    private:
        event_queue m_event_queue;

        std::atomic<bool> m_running;
        std::thread       m_render_thread;
    };

} // namespace app

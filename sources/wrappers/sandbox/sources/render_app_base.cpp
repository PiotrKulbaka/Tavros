#include "render_app_base.hpp"

#include <tavros/core/timer.hpp>

namespace app
{

    render_app_base::render_app_base(tavros::core::string_view name)
        : tavros::system::window(name)
    {
        constexpr int32 initial_width = 1280 * 2;
        constexpr int32 initial_height = 720 * 2;

        event_info initial_resize_event;
        initial_resize_event.type = event_type::window_resize;
        initial_resize_event.vec_info = tavros::math::vec2(static_cast<float>(initial_width), static_cast<float>(initial_height));
        m_event_queue.push_event(initial_resize_event);

        set_client_size(initial_width, initial_height);
    }

    render_app_base::~render_app_base()
    {
        stop_render_thread();
    }

    void render_app_base::run_render_loop()
    {
        start_render_thread();
    }

    void render_app_base::on_close(tavros::system::close_event_args& e)
    {
        stop_render_thread();
        destroy();
    }

    void render_app_base::on_activate()
    {
        event_info ei;
        ei.type = event_type::activate;
        m_event_queue.push_event(ei);
    }

    void render_app_base::on_deactivate()
    {
        event_info ei;
        ei.type = event_type::deactivate;
        m_event_queue.push_event(ei);
    }

    void render_app_base::on_resize(tavros::system::size_event_args& e)
    {
        event_info ei;
        ei.type = event_type::window_resize;
        ei.vec_info = tavros::math::vec2(static_cast<float>(e.size.width), static_cast<float>(e.size.height));
        ei.event_time_us = e.event_time_us;
        m_event_queue.push_event(ei);
    }

    void render_app_base::on_mouse_down(tavros::system::mouse_event_args& e)
    {
        if (e.is_relative_move) {
            event_info ei;
            ei.type = event_type::mouse_button_down;
            ei.mouse_button_info = e.button;
            ei.event_time_us = e.event_time_us;
            m_event_queue.push_event(ei);
        }
    }

    void render_app_base::on_mouse_move(tavros::system::mouse_event_args& e)
    {
        if (e.is_relative_move) {
            event_info ei;
            ei.type = event_type::mouse_move;
            ei.vec_info = tavros::math::vec2(static_cast<float>(e.pos.x), static_cast<float>(e.pos.y));
            ei.event_time_us = e.event_time_us;
            m_event_queue.push_event(ei);
        }
    }

    void render_app_base::on_mouse_up(tavros::system::mouse_event_args& e)
    {
        if (e.is_relative_move) {
            event_info ei;
            ei.type = event_type::mouse_button_up;
            ei.mouse_button_info = e.button;
            ei.event_time_us = e.event_time_us;
            m_event_queue.push_event(ei);
        }
    }

    void render_app_base::on_key_down(tavros::system::key_event_args& e)
    {
        event_info ei;
        ei.type = event_type::key_down;
        ei.key_info = e.key;
        ei.event_time_us = e.event_time_us;
        m_event_queue.push_event(ei);
    }

    void render_app_base::on_key_up(tavros::system::key_event_args& e)
    {
        event_info ei;
        ei.type = event_type::key_up;
        ei.key_info = e.key;
        ei.event_time_us = e.event_time_us;
        m_event_queue.push_event(ei);
    }

    void render_app_base::render_thread_main()
    {
        init();

        tavros::core::timer tm;
        tm.start();
        do {
            auto elapsed = tm.elapsed_seconds();
            tm.start();
            m_event_queue.swap_queues();
            auto events = m_event_queue.front_queue();
            render(events, elapsed);
        } while (m_running.load(std::memory_order_acquire));

        shutdown();
    }

    void render_app_base::start_render_thread()
    {
        stop_render_thread();
        m_running.store(true, std::memory_order_relaxed);
        m_render_thread = std::thread([this]() { render_thread_main(); });
    }

    void render_app_base::stop_render_thread()
    {
        auto started = m_running.exchange(false, std::memory_order_relaxed);
        if (started && m_render_thread.joinable()) {
            m_render_thread.join();
        }
    }

} // namespace app

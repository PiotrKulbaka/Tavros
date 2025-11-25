#include "render_app_base.hpp"

#include <tavros/core/timer.hpp>

namespace app
{

    render_app_base::render_app_base(tavros::core::string_view name)
        : tavros::system::window(name)
    {
        constexpr int32 initial_width = 1280 * 2;
        constexpr int32 initial_height = 720 * 2;

        tavros::input::event_args initial_resize_event;
        initial_resize_event.type = tavros::input::event_type::window_size;
        initial_resize_event.vec = tavros::math::vec2(static_cast<float>(initial_width), static_cast<float>(initial_height));
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
        tavros::input::event_args a;
        a.type = tavros::input::event_type::activate;
        m_event_queue.push_event(a);
    }

    void render_app_base::on_deactivate()
    {
        tavros::input::event_args a;
        a.type = tavros::input::event_type::deactivate;
        m_event_queue.push_event(a);
    }

    void render_app_base::on_move(tavros::system::move_event_args& e)
    {
        tavros::input::event_args a;
        a.type = tavros::input::event_type::window_move;
        a.time_us = e.event_time_us;
        a.vec = tavros::math::vec2(static_cast<float>(e.pos.x), static_cast<float>(e.pos.y));
        m_event_queue.push_event(a);
    }

    void render_app_base::on_resize(tavros::system::size_event_args& e)
    {
        tavros::input::event_args a;
        a.type = tavros::input::event_type::window_size;
        a.time_us = e.event_time_us;
        a.vec = tavros::math::vec2(static_cast<float>(e.size.width), static_cast<float>(e.size.height));
        m_event_queue.push_event(a);
    }

    void render_app_base::on_mouse_down(tavros::system::mouse_event_args& e)
    {
        tavros::input::event_args a;
        a.button = e.button;
        a.type = tavros::input::event_type::mouse_down;
        a.time_us = e.event_time_us;
        a.vec = tavros::math::vec2(static_cast<float>(e.pos.x), static_cast<float>(e.pos.height));
        m_event_queue.push_event(a);
    }

    void render_app_base::on_mouse_move(tavros::system::mouse_event_args& e)
    {
        tavros::input::event_args a;
        a.time_us = e.event_time_us;
        a.vec = tavros::math::vec2(static_cast<float>(e.pos.x), static_cast<float>(e.pos.y));
        a.type = e.is_relative_move ? tavros::input::event_type::mouse_move_delta : tavros::input::event_type::mouse_move;
        m_event_queue.push_event(a);
    }

    void render_app_base::on_mouse_up(tavros::system::mouse_event_args& e)
    {
        tavros::input::event_args a;
        a.button = e.button;
        a.type = tavros::input::event_type::mouse_up;
        a.time_us = e.event_time_us;
        a.vec = tavros::math::vec2(static_cast<float>(e.pos.x), static_cast<float>(e.pos.height));
        m_event_queue.push_event(a);
    }

    void render_app_base::on_mouse_wheel(tavros::system::mouse_event_args& e)
    {
        tavros::input::event_args a;
        a.type = tavros::input::event_type::mouse_wheel;
        a.time_us = e.event_time_us;
        a.vec = tavros::math::vec2(static_cast<float>(e.pos.x), static_cast<float>(e.pos.y));
        a.wheel = tavros::math::vec2(static_cast<float>(e.delta.x), static_cast<float>(e.delta.y));
        m_event_queue.push_event(a);
    }

    void render_app_base::on_key_down(tavros::system::key_event_args& e)
    {
        tavros::input::event_args a;
        a.type = tavros::input::event_type::key_down;
        a.time_us = e.event_time_us;
        a.key = e.key;
        m_event_queue.push_event(a);
    }

    void render_app_base::on_key_up(tavros::system::key_event_args& e)
    {
        tavros::input::event_args a;
        a.type = tavros::input::event_type::key_up;
        a.time_us = e.event_time_us;
        a.key = e.key;
        m_event_queue.push_event(a);
    }

    void render_app_base::on_key_press(tavros::system::key_event_args& e)
    {
        tavros::input::event_args a;
        a.repeats = e.repeats;
        a.key_char = e.key_char;
        a.time_us = e.event_time_us;
        a.type = tavros::input::event_type::key_press;
        m_event_queue.push_event(a);
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

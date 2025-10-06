#include "render_app_base.hpp"

#include <tavros/core/timer.hpp>

namespace app
{

    render_app_base::render_app_base(tavros::core::string_view name)
    {
        constexpr int32 initial_width = 1280 * 2;
        constexpr int32 initial_height = 720 * 2;

        m_app = tavros::system::interfaces::application::create();
        m_wnd = tavros::system::interfaces::window::create(name);

        m_wnd->set_window_size(initial_width, initial_height);
        center_window();

        init_window_callbacks();

        event_info initial_resize_event;
        initial_resize_event.type = event_type::window_resize;
        initial_resize_event.vec_info = tavros::math::vec2(static_cast<float>(initial_width), static_cast<float>(initial_height));
        m_event_queue.push_event(initial_resize_event);
    }

    render_app_base::~render_app_base()
    {
    }

    void* render_app_base::native_window_handle() const noexcept
    {
        return m_wnd->native_handle();
    }

    int render_app_base::run()
    {
        m_app->run();
        m_app->poll_events();

        start_render_thread();

        m_wnd->show();

        while (m_app->is_runing()) {
            m_app->wait_events();
            m_app->poll_events();
        }

        stop_render_thread();

        return 0;
    }

    void render_app_base::center_window()
    {
        auto ds = m_app->desktop_size();
        auto ws = m_wnd->get_window_size();
        auto loc = (ds - ws) / 2;
        m_wnd->set_location(loc.left, loc.top);
    }

    void render_app_base::init_window_callbacks()
    {
        m_wnd->set_on_close_listener([&](tavros::system::window_ptr, tavros::system::close_event_args& e) {
            m_app->exit();
        });

        m_wnd->set_on_resize_listener([&](tavros::system::window_ptr, tavros::system::size_event_args& e) {
            event_info ei;
            ei.type = event_type::window_resize;
            ei.vec_info = tavros::math::vec2(static_cast<float>(e.size.width), static_cast<float>(e.size.height));
            m_event_queue.push_event(ei);
        });

        m_wnd->set_on_key_down_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
            event_info ei;
            ei.type = event_type::key_down;
            ei.key_info = e.key;
            m_event_queue.push_event(ei);
        });

        m_wnd->set_on_key_up_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
            event_info ei;
            ei.type = event_type::key_up;
            ei.key_info = e.key;
            m_event_queue.push_event(ei);
        });

        m_wnd->set_on_deactivate_listener([&](tavros::system::window_ptr) {
            event_info ei;
            ei.type = event_type::deactivate;
            m_event_queue.push_event(ei);
        });

        m_wnd->set_on_activate_listener([&](tavros::system::window_ptr) {
            event_info ei;
            ei.type = event_type::activate;
            m_event_queue.push_event(ei);
        });

        m_wnd->set_on_mouse_move_listener([&](tavros::system::window_ptr, tavros::system::mouse_event_args& e) {
            if (e.is_relative_move) {
                event_info ei;
                ei.type = event_type::mouse_move;
                ei.vec_info = tavros::math::vec2(static_cast<float>(e.pos.x), static_cast<float>(e.pos.y));
                m_event_queue.push_event(ei);
            }
        });

        m_wnd->set_on_mouse_down_listener([&](tavros::system::window_ptr, tavros::system::mouse_event_args& e) {
            if (e.is_relative_move) {
                event_info ei;
                ei.type = event_type::mouse_button_down;
                ei.mouse_button_info = e.button;
                m_event_queue.push_event(ei);
            }
        });

        m_wnd->set_on_mouse_up_listener([&](tavros::system::window_ptr, tavros::system::mouse_event_args& e) {
            if (e.is_relative_move) {
                event_info ei;
                ei.type = event_type::mouse_button_up;
                ei.mouse_button_info = e.button;
                m_event_queue.push_event(ei);
            }
        });
    }

    void render_app_base::render_thread_main()
    {
        init();

        tavros::core::timer tm;
        tm.start();
        do {
            float elapsed = tm.elapsed_seconds();
            tm.start();
            m_event_queue.swap_queues();
            auto events = m_event_queue.front_queue();
            render(events, elapsed);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } while (m_running.load(std::memory_order_acquire));

        shutdown();
    }

    void render_app_base::start_render_thread()
    {
        m_running.store(true, std::memory_order_relaxed);
        m_render_thread = std::thread([this]() { render_thread_main(); });
    }

    void render_app_base::stop_render_thread()
    {
        m_running.store(false, std::memory_order_relaxed);
        if (m_render_thread.joinable()) {
            m_render_thread.join();
        }
    }

} // namespace app

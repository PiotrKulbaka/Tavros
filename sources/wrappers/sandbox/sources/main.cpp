#include <iostream>

#include <tavros/core/prelude.hpp>
#include <tavros/core/timer.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/math/utils/make_string.hpp>

#include <tavros/system/interfaces/window.hpp>
#include <tavros/system/interfaces/application.hpp>

#include <inttypes.h>

#include <thread>

int main()
{
    namespace core = tavros::core;

    tavros::core::logger::add_consumer([](core::severity_level, core::string_view tag, core::string_view msg) {
        TAV_ASSERT(tag.data());
        std::cout << msg << std::endl;
    });

    auto logger = tavros::core::logger("main");

    auto app = tavros::system::interfaces::application::create();

    auto wnd = tavros::system::interfaces::window::create("TavrosEngine");
    wnd->set_window_size(1280, 720);
    wnd->set_location(100, 100);

    wnd->show();
    wnd->set_on_close_listener([&](tavros::system::window_ptr, tavros::system::close_event_args& e) {
        logger.info("Before exit.");
        app->exit();
        logger.info("After exit.");
    });


    wnd->set_on_mouse_down_listener([&](tavros::system::window_ptr, tavros::system::mouse_event_args& e) {
        logger.info("on_mouse_down: %s: %f %f", tavros::system::to_string(e.button).data(), e.pos.x, e.pos.y);
    });

    wnd->set_on_mouse_move_listener([&](tavros::system::window_ptr, tavros::system::mouse_event_args& e) {
        if (e.is_relative_move) {
            //            logger.info("on_mouse_move: %f %f", e.pos.x, e.pos.y);
        }
    });

    wnd->set_on_mouse_up_listener([&](tavros::system::window_ptr, tavros::system::mouse_event_args& e) {
        logger.info("on_mouse_up: %s: %f %f", tavros::system::to_string(e.button).data(), e.pos.x, e.pos.y);
    });

    wnd->set_on_mouse_wheel_listener([&](tavros::system::window_ptr, tavros::system::mouse_event_args& e) {
        logger.info("on_mouse_up: wheel_x: %f; wheel_y: %f; %f %f", e.delta.x, e.delta.y, e.pos.x, e.pos.y);
    });

    wnd->set_on_key_down_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
        logger.info("on_key_down: %s", tavros::system::to_string(e.key).data());
    });

    wnd->set_on_key_up_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
        logger.info("on_key_up: %c", tavros::system::to_string(e.key).data());
    });

    wnd->set_on_key_press_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
        logger.info("on_key_press: %c", tavros::system::to_string(e.key).data());
    });

    wnd->set_on_activate_listener([&](tavros::system::window_ptr) {
        logger.info("on_activate");
    });

    wnd->set_on_deactivate_listener([&](tavros::system::window_ptr) {
        logger.info("on_deactivate");
    });

    wnd->set_on_move_listener([&](tavros::system::window_ptr, tavros::system::move_event_args& e) {
        logger.info("on_move: %d %d", e.pos.x, e.pos.y);
    });

    wnd->set_on_resize_listener([&](tavros::system::window_ptr, tavros::system::size_event_args& e) {
        logger.info("on_resize: %d %d", e.size.width, e.size.height);
    });

    app->run();

    while (app->is_runing()) {
        app->poll_events();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    logger.info("Will exit.");

    return 0;
}

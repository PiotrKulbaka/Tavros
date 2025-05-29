#include <iostream>

#include <tavros/core/prelude.hpp>
#include <tavros/core/timer.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/math/utils/make_string.hpp>

#include <tavros/system/interfaces/window.hpp>
#include <tavros/system/interfaces/application.hpp>
#include <tavros/renderer/interfaces/gl_context.hpp>
#include <tavros/renderer/camera/camera.hpp>
#include <tavros/renderer/shader.hpp>

#include <glad/glad.h>

#include <inttypes.h>

#include <thread>

float vertices[] = {
    // positions
    0.0f, 0.5f, 0.0f,   // Top
    -0.5f, -0.5f, 0.0f, // Bottom left
    0.5f, -0.5f, 0.0f   // Bottom right
};

const char* vertex_shader_source = R"(
#version 410 core
layout (location = 0) in vec3 a_pos;

uniform mat4 u_mvp;

void main()
{
    gl_Position = u_mvp * vec4(a_pos, 1.0);
}
)";

const char* fragment_shader_source = R"(
#version 410 core
out vec4 frag_color;

void main()
{
    frag_color = vec4(1.0, 0.4, 0.2, 1.0); // оранжевый
}
)";

int main()
{
    namespace core = tavros::core;

    tavros::core::logger::add_consumer([](core::severity_level, core::string_view tag, core::string_view msg) {
        TAV_ASSERT(tag.data());
        std::cout << msg << std::endl;
    });

    auto logger = tavros::core::logger("main");

    tavros::renderer::camera cam;
    bool                     key_state[static_cast<int32>(tavros::system::keys::k_last_key)];

    constexpr uint8 k_up = static_cast<uint8>(tavros::system::keys::k_W);
    constexpr uint8 k_right = static_cast<uint8>(tavros::system::keys::k_D);
    constexpr uint8 k_left = static_cast<uint8>(tavros::system::keys::k_A);
    constexpr uint8 k_down = static_cast<uint8>(tavros::system::keys::k_S);


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
            //            tavros::math::quat q(cam.up(), e.);
            //            cam.rotate(<#const math::quat &q#>);
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
        //        logger.info("on_key_down: %s", tavros::system::to_string(e.key).data());
        key_state[static_cast<int32>(e.key)] = true;
    });

    wnd->set_on_key_up_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
        //        logger.info("on_key_up: %c", tavros::system::to_string(e.key).data());
        key_state[static_cast<int32>(e.key)] = false;
    });

    wnd->set_on_key_press_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
        //        logger.info("on_key_press: %c", tavros::system::to_string(e.key).data());
    });

    wnd->set_on_activate_listener([&](tavros::system::window_ptr) {
        //        logger.info("on_activate");
        memset(&key_state, 0, sizeof(key_state));
    });

    wnd->set_on_deactivate_listener([&](tavros::system::window_ptr) {
        //        logger.info("on_deactivate");
        memset(&key_state, 0, sizeof(key_state));
    });

    wnd->set_on_move_listener([&](tavros::system::window_ptr, tavros::system::move_event_args& e) {
        //        logger.info("on_move: %d %d", e.pos.x, e.pos.y);
    });

    wnd->set_on_resize_listener([&](tavros::system::window_ptr, tavros::system::size_event_args& e) {
        //        logger.info("on_resize: %d %d", e.size.width, e.size.height);
        cam.set_perspective(tavros::math::k_pi / 3.0f, e.size.width / e.size.height, 0.1f, 1000.0f);
    });

    cam.set_position({-10.0f, 0.0f, 0.0f});

    app->run();

    auto context = tavros::renderer::interfaces::gl_context::create(wnd->get_handle());

    context->make_current();

    if (!gladLoadGL()) {
        throw std::runtime_error("Failed to initialize OpenGL context via GLAD");
    }


    auto sh = tavros::renderer::shader::create(vertex_shader_source, fragment_shader_source);


    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    GLint u_mvp = glGetUniformLocation(sh.handle(), "u_mvp");


    tavros::core::timer tm;

    while (app->is_runing()) {
        auto f_time = static_cast<float>(tm.elapsed<std::chrono::microseconds>()) / 1000000.0f;
        if (f_time > 1.0f) {
            f_time = 1.0f;
        }
        tm.start();
        app->poll_events();


        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        sh.use();

        auto vp = tavros::math::mat4::identity(); // cam.get_view_projection_matrix();
        glUniformMatrix4fv(u_mvp, 1, true, vp.data());

        glBindVertexArray(vao);

        glDrawArrays(GL_TRIANGLES, 0, 3);


        if (key_state[k_up]) {
            cam.move(cam.forward() * f_time * 4.0f);
            logger.info("cam_pos: %s", tavros::core::make_string(cam.position(), 2).c_str());

            logger.info("cam_vp: %s", tavros::core::make_string(vp, 2).c_str());
        }

        if (key_state[k_down]) {
            cam.move(-cam.forward() * f_time * 4.0f);
            logger.info("cam_pos: %s", tavros::core::make_string(cam.position(), 2).c_str());

            logger.info("cam_vp: %s", tavros::core::make_string(vp, 2).c_str());
        }

        if (key_state[k_right]) {
            cam.move(cam.right() * f_time * 4.0f);
            logger.info("cam_pos: %s", tavros::core::make_string(cam.position(), 2).c_str());

            logger.info("cam_vp: %s", tavros::core::make_string(vp, 2).c_str());
        }

        if (key_state[k_left]) {
            cam.move(-cam.right() * f_time * 4.0f);
            logger.info("cam_pos: %s", tavros::core::make_string(cam.position(), 2).c_str());

            logger.info("cam_vp: %s", tavros::core::make_string(vp, 2).c_str());
        }

        context->swap_buffers();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    logger.info("Will exit.");

    return 0;
}

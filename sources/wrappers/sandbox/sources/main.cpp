#include <iostream>

#include <tavros/core/prelude.hpp>
#include <tavros/core/timer.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/math/utils/make_string.hpp>

#include <tavros/system/interfaces/window.hpp>
#include <tavros/system/interfaces/application.hpp>
#include <tavros/renderer/interfaces/gl_context.hpp>

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
#version 330 core
layout (location = 0) in vec3 a_pos;

void main()
{
    gl_Position = vec4(a_pos, 1.0);
}
)";

const char* fragment_shader_source = R"(
#version 330 core
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

    auto context = tavros::renderer::interfaces::gl_context::create(wnd->get_handle());

    context->make_current();

    if (!gladLoadGL()) {
        throw std::runtime_error("Failed to initialize OpenGL context via GLAD");
    }

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
    glCompileShader(fragment_shader);

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

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


    while (app->is_runing()) {
        app->poll_events();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);


        context->swap_buffers();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    logger.info("Will exit.");

    return 0;
}

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

#include <tavros/renderer/rhi/command_list.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

#include <tavros/renderer/internal/backend/gl/gl_command_list.hpp>
#include <tavros/renderer/internal/backend/gl/gl_graphics_device.hpp>

#include <glad/glad.h>

#include <inttypes.h>

#include <thread>

#include <stb/stb_image.h>

float vertices[] = {
    // positions        // texcoords
    0.0f, 0.5f, 0.0f, 0.0f, 0.0f,  // Top
    -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // Left
    0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // Right

    0.0f, -0.5f, 0.0f, 1.0f, 1.0f, // Bottom
    -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // Left
    0.5f, 0.0f, 0.0f, 1.0f, 0.0f   // Right
};

const char* vertex_shader_source = R"(
#version 420 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_uv;

out vec2 v_uv;

void main()
{
    gl_Position = vec4(a_pos, 1.0);
    v_uv = a_uv;
}
)";

const char* fragment_shader_source = R"(
#version 420 core

layout(binding = 0) uniform sampler2D u_tex1;
layout(binding = 2) uniform sampler2D u_tex2;
in vec2 v_uv;
out vec4 frag_color;

void main()
{
    vec4 top = texture(u_tex2, v_uv);
    vec4 bottom = texture(u_tex1, v_uv);
    frag_color = bottom + top * (1.0 - bottom.a);
}
)";


void* load_pixels_from_file(const char* filename, int32& w, int32& h, int32& c)
{
    return static_cast<void*>(stbi_load(filename, &w, &h, &c, STBI_rgb_alpha));
}

void free_pixels(void* p)
{
    stbi_image_free(p);
}

int main()
{
    tavros::core::logger::add_consumer([](tavros::core::severity_level, tavros::core::string_view tag, tavros::core::string_view msg) {
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
        app->exit();
    });

    auto context = tavros::renderer::interfaces::gl_context::create(wnd->get_handle());

    context->make_current();

    if (!gladLoadGL()) {
        throw std::runtime_error("Failed to initialize OpenGL context via GLAD");
    }


    app->run();

    tavros::renderer::pipeline_desc ppl;

    ppl.shaders.fragment_source = fragment_shader_source;
    ppl.shaders.vertex_source = vertex_shader_source;

    ppl.depth_stencil.depth_test_enable = true;
    ppl.depth_stencil.depth_write_enable = true;
    ppl.depth_stencil.depth_compare = tavros::renderer::compare_op::less;

    ppl.rasterizer.cull = tavros::renderer::cull_face::off;
    ppl.rasterizer.polygon = tavros::renderer::polygon_mode::fill;


    auto gdevice = tavros::core::make_shared<tavros::renderer::gl_graphics_device>();
    auto comlist = tavros::core::make_shared<tavros::renderer::gl_command_list>(gdevice.get());


    tavros::renderer::texture2d_desc tex_desc;

    int32 w, h, c;
    void* pixels = load_pixels_from_file("D:\\Work\\q3pp_res\\baseq3\\textures\\base_support\\x_support3.tga", w, h, c);
    tex_desc.data = pixels;
    tex_desc.width = w;
    tex_desc.height = h;
    tex_desc.mip_levels = 1;

    auto tex1 = gdevice->create_texture(tex_desc);
    free_pixels(pixels);

    pixels = load_pixels_from_file("D:\\Work\\q3pp_res\\baseq3\\textures\\base_wall\\metalfloor_wall_14_specular.tga", w, h, c);
    tex_desc.data = pixels;
    tex_desc.width = w;
    tex_desc.height = h;
    tex_desc.mip_levels = 1;

    auto tex2 = gdevice->create_texture(tex_desc);
    free_pixels(pixels);


    tavros::renderer::sampler_desc samler_desc;
    samler_desc.filter.mipmap_filter = tavros::renderer::mipmap_filter_mode::off;
    samler_desc.filter.min_filter = tavros::renderer::filter_mode::linear;
    samler_desc.filter.mag_filter = tavros::renderer::filter_mode::linear;

    auto sampler1 = gdevice->create_sampler(samler_desc);
    auto pipeline = gdevice->create_pipeline(ppl);


    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Создаём один UBO
    /*constexpr size_t k_big_ubo_size = 4096;
    GLuint big_ubo;
    glGenBuffers(1, &big_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, big_ubo);
    glBufferData(GL_UNIFORM_BUFFER, k_big_ubo_size, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Обновляем часть буфера
    glBindBuffer(GL_UNIFORM_BUFFER, big_ubo);
    constexpr auto offset_in_ubo = 0;
    //glBufferSubData(GL_UNIFORM_BUFFER, offset_in_ubo, sizeof(GlobalData), &global_data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    */
    /*
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, big_ubo, offset_global, sizeof(GlobalData)); // binding = 0
    glBindBufferRange(GL_UNIFORM_BUFFER, 1, big_ubo, offset_object, sizeof(ObjectData)); // binding = 1
    */

    while (app->is_runing()) {
        app->poll_events();

        glClearDepth(1.0);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        comlist->bind_pipeline(pipeline);

        auto binding = 0;
        glActiveTexture(GL_TEXTURE0 + binding);
        glBindTexture(GL_TEXTURE_2D, tex1.id);
        glBindSampler(binding, sampler1.id);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, tex2.id);
        glBindSampler(2, sampler1.id);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        context->swap_buffers();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

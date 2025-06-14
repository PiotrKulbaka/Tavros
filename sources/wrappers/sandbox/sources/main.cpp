#include <iostream>

#include <tavros/core/prelude.hpp>
#include <tavros/core/timer.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/math/utils/make_string.hpp>

#include <tavros/system/interfaces/window.hpp>
#include <tavros/system/interfaces/application.hpp>
#include <tavros/renderer/interfaces/gl_context.hpp>
#include <tavros/renderer/camera/camera.hpp>

#include <tavros/renderer/rhi/command_list.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

#include <tavros/renderer/internal/opengl/command_list_opengl.hpp>
#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

#include <tavros/core/containers/static_vector.hpp>

#include <glad/glad.h>

#include <inttypes.h>

#include <thread>

#include <tavros/core/scoped_owner.hpp>

#include <tavros/renderer/rhi/geometry_binding_desc.hpp>

#include <stb/stb_image.h>

// clang-format off
float cube_vertices[] = {
    // X      Y      Z    pad      U      V    pad    pad
    // Front face (Z+)
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, // bottom-left
     0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, // bottom-right
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f, // top-right

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, // bottom-left
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f, // top-right
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, // top-left

    // Back face (Z-)
    -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, // bottom-right
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, // bottom-left
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, // top-left

    -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, // bottom-right
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, // top-left
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f, // top-right

    // Left face (X-)
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    // Right face (X+)
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,

     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    // Top face (Y+)
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,

    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    // Bottom face (Y-)
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
};

float cube_colors[] =
{
    // Front face (red)
    1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f,

    1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f,

    // Back face (green)
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,

    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,

    // Left face (blue)
    0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,

    0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,

    // Right face (yellow)
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,

    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,

    // Top face (purple)
    0.7f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, 1.0f, 1.0f,

    0.7f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, 1.0f, 1.0f,

    // Bottom face (white blue)
    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,

    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
};

uint16 cube_indices[] =
{
     0,  1,  2,  3,  4,  5, // Front face
     6,  7,  8,  9, 10, 11, // Back face
    12, 13, 14, 15, 16, 17, // Left face
    18, 19, 20, 21, 22, 23, // Right face
    24, 25, 26, 27, 28, 29, // Top face
    30, 31, 32, 33, 34, 35, // Bottom face
};

// clang-format on

const char* vertex_shader_source = R"(
#version 420 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

out vec2 v_uv;
out vec4 v_color;

layout (binding = 0) uniform Camera
{
    mat4 u_camera;
};

void main()
{
    gl_Position = u_camera * vec4(a_pos, 1.0);
    v_uv = a_uv;
    v_color = a_color;
}
)";

const char* fragment_shader_source = R"(
#version 420 core

layout(binding = 0) uniform sampler2D u_tex1;
layout(binding = 2) uniform sampler2D u_tex2;

in vec2 v_uv;
in vec4 v_color;

out vec4 frag_color;

void main()
{
    vec4 top = texture(u_tex2, v_uv);
    vec4 bottom = texture(u_tex1, v_uv);
    float a = bottom.a + top.a * (1.0 - bottom.a);


    frag_color = mix(bottom + top * (1.0 - bottom.a), v_color, 0.1 * a);
    frag_color.a = a;
}
)";


uint8* load_pixels_from_file(const char* filename, int32& w, int32& h, int32& c)
{
    return static_cast<uint8*>(stbi_load(filename, &w, &h, &c, STBI_rgb_alpha));
}

void free_pixels(uint8* p)
{
    stbi_image_free(p);
}

void update_cameta(const bool* keys, tavros::math::vec2 mouse_delta, float elapsed, float aspect_ratio, tavros::renderer::camera& cam)
{
    tavros::math::vec3 dir;
    constexpr float    k_speed_factor = 2.0f;

    if (elapsed > 1.0f) {
        elapsed = 1.0f;
    }

    if (keys[(uint8) tavros::system::keys::k_W]) {
        dir += cam.forward();
    }

    if (keys[(uint8) tavros::system::keys::k_S]) {
        dir -= cam.forward();
    }

    if (keys[(uint8) tavros::system::keys::k_D]) {
        dir += cam.right();
    }

    if (keys[(uint8) tavros::system::keys::k_A]) {
        dir -= cam.right();
    }

    if (keys[(uint8) tavros::system::keys::k_space]) {
        dir += cam.up();
    }

    if (keys[(uint8) tavros::system::keys::k_C]) {
        dir -= cam.up();
    }

    if (tavros::math::squared_length(dir) > 0.0f) {
        auto norm = tavros::math::normalize(dir);
        cam.move(norm * elapsed * k_speed_factor);
    }

    if (tavros::math::squared_length(mouse_delta) > 0.0f) {
        auto m = (mouse_delta / 5.0f) * elapsed;

        auto q_pitch = tavros::math::quat::from_axis_angle(cam.right(), -m.y);
        auto q_yaw = tavros::math::quat::from_axis_angle(tavros::math::vec3{0.0f, 1.0f, 0.0f}, -m.x);
        auto rotation = tavros::math::normalize(q_yaw * q_pitch);

        cam.set_orientation(rotation * cam.forward(), rotation * cam.up());
    }

    cam.set_perspective(3.14159265358979f / 3.0f, aspect_ratio, 0.1f, 1000.0f);
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

    float aspect_ratio = 1280.0f / 720.0f;

    wnd->set_on_resize_listener([&](tavros::system::window_ptr, tavros::system::size_event_args& e) {
        aspect_ratio = static_cast<float>(e.size.width) / static_cast<float>(e.size.height);
        glViewport(0, 0, e.size.width, e.size.height);
    });

    static bool keys[256] = {false};

    wnd->set_on_key_down_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
        keys[(uint8) e.key] = true;
    });

    wnd->set_on_key_up_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
        keys[(uint8) e.key] = false;
    });

    wnd->set_on_deactivate_listener([&](tavros::system::window_ptr) {
        for (auto& v : keys) {
            v = false;
        }
    });

    wnd->set_on_activate_listener([&](tavros::system::window_ptr) {
        for (auto& v : keys) {
            v = false;
        }
    });

    tavros::math::point2 mouse_delta;

    wnd->set_on_mouse_move_listener([&](tavros::system::window_ptr, tavros::system::mouse_event_args& e) {
        if (e.is_relative_move) {
            mouse_delta = e.pos;
        }
    });

    app->run();


    auto gdevice = tavros::core::make_shared<tavros::renderer::graphics_device_opengl>();
    auto comlist = tavros::core::make_shared<tavros::renderer::command_list_opengl>(gdevice.get());

    tavros::renderer::swapchain_desc main_swapchain_desc;
    main_swapchain_desc.width = wnd->get_client_size().width;
    main_swapchain_desc.height = wnd->get_client_size().height;
    main_swapchain_desc.buffer_count = 3;
    main_swapchain_desc.vsync = true;
    main_swapchain_desc.color_attachment.format = tavros::renderer::pixel_format::rgba8un;
    main_swapchain_desc.color_attachment.load = tavros::renderer::load_op::clear;
    main_swapchain_desc.color_attachment.store = tavros::renderer::store_op::store;
    main_swapchain_desc.color_attachment.clear_value[0] = 0.1f;
    main_swapchain_desc.color_attachment.clear_value[1] = 0.2f;
    main_swapchain_desc.color_attachment.clear_value[2] = 0.3f;
    main_swapchain_desc.color_attachment.clear_value[3] = 1.0f;
    main_swapchain_desc.depth_stencil_attachment.format = tavros::renderer::pixel_format::depth24_stencil8;
    main_swapchain_desc.depth_stencil_attachment.load = tavros::renderer::load_op::clear;
    main_swapchain_desc.depth_stencil_attachment.store = tavros::renderer::store_op::dont_care;
    main_swapchain_desc.depth_stencil_attachment.depth_clear_value = 0.0f;
    main_swapchain_desc.depth_stencil_attachment.stencil_clear_value = 0;

    auto main_swapchain_handle = gdevice->create_swapchain(main_swapchain_desc, wnd->get_handle());

    tavros::renderer::texture_desc tex_desc;

    int32 w, h, c;
    auto* pixels = load_pixels_from_file("D:\\Work\\q3pp_res\\baseq3\\textures\\base_support\\x_support3.tga", w, h, c);
    tex_desc.width = w;
    tex_desc.height = h;
    tex_desc.mip_levels = 1;

    auto tex1 = gdevice->create_texture(tex_desc, pixels);
    free_pixels(pixels);

    pixels = load_pixels_from_file("D:\\Work\\q3pp_res\\baseq3\\textures\\base_wall\\metalfloor_wall_14_specular.tga", w, h, c);
    tex_desc.width = w;
    tex_desc.height = h;
    tex_desc.mip_levels = 1;

    auto tex2 = gdevice->create_texture(tex_desc, pixels);
    free_pixels(pixels);


    tavros::renderer::texture_desc atch_desc;
    atch_desc.width = 1280;
    atch_desc.height = 720;
    atch_desc.format = tavros::renderer::pixel_format::rgba8un;
    auto atch1 = gdevice->create_texture(atch_desc);

    atch_desc.format = tavros::renderer::pixel_format::rgb32f;
    auto atch2 = gdevice->create_texture(atch_desc);

    atch_desc.format = tavros::renderer::pixel_format::depth24_stencil8;
    auto atch_ds = gdevice->create_texture(atch_desc);

    tavros::renderer::framebuffer_desc fb_desc;
    fb_desc.width = 1280;
    fb_desc.height = 720;
    fb_desc.color_attachments.push_back(
        {tavros::renderer::pixel_format::rgba8un,
         tavros::renderer::load_op::clear,
         tavros::renderer::store_op::store,
         {1.0f, 0.0f, 0.0f, 1.0f}}
    );

    fb_desc.color_attachments.push_back(
        {tavros::renderer::pixel_format::rgb32f,
         tavros::renderer::load_op::clear,
         tavros::renderer::store_op::store,
         {1.0f, 1.0f, 1.0f, 0.0f}}
    );

    fb_desc.depth_stencil_attachment = {
        tavros::renderer::pixel_format::depth24_stencil8,
        tavros::renderer::load_op::clear,
        tavros::renderer::store_op::dont_care,
        1.0f,
        0
    };


    tavros::renderer::texture_handle attachments[] = {atch1, atch2};

    auto fb = gdevice->create_framebuffer(fb_desc, attachments, atch_ds);


    tavros::renderer::sampler_desc samler_desc;
    samler_desc.filter.mipmap_filter = tavros::renderer::mipmap_filter_mode::off;
    samler_desc.filter.min_filter = tavros::renderer::filter_mode::linear;
    samler_desc.filter.mag_filter = tavros::renderer::filter_mode::linear;

    auto sampler1 = gdevice->create_sampler(samler_desc);


    tavros::renderer::pipeline_desc main_pipeline_desc;

    main_pipeline_desc.shaders.fragment_source = fragment_shader_source;
    main_pipeline_desc.shaders.vertex_source = vertex_shader_source;

    main_pipeline_desc.depth_stencil.depth_test_enable = true;
    main_pipeline_desc.depth_stencil.depth_write_enable = true;
    main_pipeline_desc.depth_stencil.depth_compare = tavros::renderer::compare_op::less;

    main_pipeline_desc.rasterizer.cull = tavros::renderer::cull_face::off;
    main_pipeline_desc.rasterizer.polygon = tavros::renderer::polygon_mode::fill;


    auto main_pipeline = gdevice->create_pipeline(main_pipeline_desc);


    namespace r = tavros::renderer;
    r::buffer_desc xyz_uv_desc;
    xyz_uv_desc.size = 1024 * 128; // 128 KiB
    xyz_uv_desc.usage = r::buffer_usage::vertex;

    auto buffer_xyz_uv = gdevice->create_buffer(xyz_uv_desc, (uint8*) cube_vertices, sizeof(cube_vertices));

    r::buffer_desc rgba_desc;
    xyz_uv_desc.size = 1024 * 128; // 128 KiB
    xyz_uv_desc.usage = r::buffer_usage::vertex;

    auto buffer_rgba = gdevice->create_buffer(xyz_uv_desc, (uint8*) cube_colors, sizeof(cube_colors));

    r::buffer_desc indices_desc;
    indices_desc.size = 1024 * 128; // 128 KiB
    indices_desc.usage = r::buffer_usage::index;

    auto buffer_indices = gdevice->create_buffer(indices_desc, (uint8*) cube_indices, sizeof(cube_indices));


    r::geometry_binding_desc gbd;
    gbd.layout.attributes.push_back({3, r::attribute_format::f32, false});
    gbd.layout.attributes.push_back({2, r::attribute_format::f32, false});
    gbd.layout.attributes.push_back({4, r::attribute_format::f32, false});

    gbd.attribute_mapping.push_back({0, 0});
    gbd.attribute_mapping.push_back({0, 4 * 4});
    gbd.attribute_mapping.push_back({1, 0});

    gbd.buffer_mapping.push_back({0, 0, 4 * 8});
    gbd.buffer_mapping.push_back({1, 0, 4 * 4});

    gbd.has_index_buffer = true;
    gbd.index_format = r::index_buffer_format::u16;

    r::buffer_handle buffers_to_binding[] = {buffer_xyz_uv, buffer_rgba};
    auto             geometry1 = gdevice->create_geometry(gbd, buffers_to_binding, buffer_indices);

    auto cam = tavros::renderer::camera({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0});


    GLuint ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, 1024, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    tavros::core::timer tm;

    while (app->is_runing()) {
        app->poll_events();

        auto* main_swapchain_ptr = gdevice->get_swapchain_ptr_by_handle(main_swapchain_handle);
        auto  backbuffer_index = main_swapchain_ptr->acquire_next_backbuffer_index();
        auto  main_framebuffer = main_swapchain_ptr->get_framebuffer(backbuffer_index);

        float elapsed = tm.elapsed<std::chrono::microseconds>() / 1000000.0f;
        tm.start();

        update_cameta(keys, mouse_delta, elapsed, aspect_ratio, cam);
        mouse_delta = tavros::math::vec2();


        glClearDepth(1.0);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        comlist->bind_pipeline(main_pipeline);

        comlist->bind_geometry(geometry1);

        auto binding = 0;
        glActiveTexture(GL_TEXTURE0 + binding);
        glBindTexture(GL_TEXTURE_2D, tex1.id);
        glBindSampler(binding, sampler1.id);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, tex2.id);
        glBindSampler(2, sampler1.id);


        auto cam_mat = cam.get_view_projection_matrix();
        cam_mat = tavros::math::transpose(cam_mat);

        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(tavros::math::mat4), cam_mat.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);


        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

        main_swapchain_ptr->present(backbuffer_index);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

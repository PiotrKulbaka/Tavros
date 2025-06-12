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

#include <tavros/renderer/internal/backend/gl/gl_command_list.hpp>
#include <tavros/renderer/internal/backend/gl/gl_graphics_device.hpp>

#include <tavros/core/containers/static_vector.hpp>

#include <glad/glad.h>

#include <inttypes.h>

#include <thread>

#include <tavros/core/scoped_owner.hpp>

#include <tavros/renderer/rhi/geometry_binding_desc.hpp>

#include <stb/stb_image.h>

float vertices[] = {
    // positions        // texcoords
    0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0, 0.0, 0.0, 1.0,  // Top
    -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0, 1.0, 0.0, 1.0, // Left
    0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0, 0.0, 1.0, 1.0,  // Right

    0.0f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0, 0.0, 1.0, 1.0, // Bottom
    -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0, 1.0, 1.0, 1.0, // Left
    0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0, 1.0, 0.0, 1.0,  // Right
};

const char* vertex_shader_source = R"(
#version 420 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

out vec2 v_uv;
out vec4 v_color;

void main()
{
    gl_Position = vec4(a_pos, 1.0);
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
    frag_color = mix(bottom + top * (1.0 - bottom.a), v_color, 0.5);
}
)";

const char* vertex_shader_source2 = R"(
#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 frag_uv;

void main()
{
    frag_uv = in_uv;
    gl_Position = vec4(in_position, 1.0);
}
)";

const char* fragment_shader_source2 = R"(
#version 450 core

in vec2 frag_uv;

// ѕервый вывод Ч в GL_COLOR_ATTACHMENT0 (rgba8un)
layout(location = 0) out vec4 out_color0;

// ¬торой вывод Ч в GL_COLOR_ATTACHMENT1 (rgb32f)
layout(location = 1) out vec3 out_color1;

void main()
{
    // ѕросто градиенты дл€ теста
    out_color0 = vec4(frag_uv, 0.0, 1.0);              // RG в виде UV
    out_color1 = vec3(frag_uv.x, frag_uv.y, 1.0);      // RGB float градиент
}
)";

const char* vertex_shader_source_blit = R"(
#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 frag_uv;

void main()
{
    frag_uv = in_uv;
    gl_Position = vec4(in_position, 1.0);
}
)";

const char* fragment_shader_source_blit = R"(
#version 450 core

in vec2 frag_uv;

// —амый обычный вывод в экранный фреймбуфер (GL_BACK)
out vec4 out_color;

// ƒве текстуры, полученные из предыдущего прохода
layout(binding = 0) uniform sampler2D tex0; // rgba8un
layout(binding = 0) uniform sampler2D tex1; // rgb32f

void main()
{
    vec4 color0 = texture(tex0, frag_uv); // из GL_COLOR_ATTACHMENT0
    vec3 color1 = texture(tex1, frag_uv); // из GL_COLOR_ATTACHMENT1

    // ѕример комбинировани€ двух текстур: наложение и €ркость
    vec3 result_rgb = color0.rgb * 0.5 + color1.rgb * 0.5;
    out_color = vec4(result_rgb, 1.0);
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


    tavros::renderer::pipeline_desc pipeline_d;

    pipeline_d.shaders.fragment_source = fragment_shader_source;
    pipeline_d.shaders.vertex_source = vertex_shader_source;

    pipeline_d.depth_stencil.depth_test_enable = true;
    pipeline_d.depth_stencil.depth_write_enable = true;
    pipeline_d.depth_stencil.depth_compare = tavros::renderer::compare_op::less;

    pipeline_d.rasterizer.cull = tavros::renderer::cull_face::off;
    pipeline_d.rasterizer.polygon = tavros::renderer::polygon_mode::fill;

    auto pipeline2 = gdevice->create_pipeline(pipeline_d);


    tavros::renderer::pipeline_desc ppl_blit;

    ppl_blit.shaders.fragment_source = fragment_shader_source_blit;
    ppl_blit.shaders.vertex_source = vertex_shader_source_blit;

    ppl_blit.depth_stencil.depth_test_enable = false;
    ppl_blit.depth_stencil.depth_write_enable = false;

    ppl_blit.rasterizer.cull = tavros::renderer::cull_face::off;
    ppl_blit.rasterizer.polygon = tavros::renderer::polygon_mode::fill;

    auto pipeline_blit = gdevice->create_pipeline(ppl_blit);


    tavros::renderer::sampler_desc samler_desc;
    samler_desc.filter.mipmap_filter = tavros::renderer::mipmap_filter_mode::off;
    samler_desc.filter.min_filter = tavros::renderer::filter_mode::linear;
    samler_desc.filter.mag_filter = tavros::renderer::filter_mode::linear;

    auto sampler1 = gdevice->create_sampler(samler_desc);
    auto pipeline = gdevice->create_pipeline(ppl);


    namespace r = tavros::renderer;
    r::buffer_desc bd;
    bd.size = sizeof(vertices);
    bd.usage = r::buffer_usage::vertex;

    auto buffer1 = gdevice->create_buffer(bd, (uint8*) vertices, sizeof(vertices));


    r::geometry_binding_desc gbd;
    gbd.layout.attributes.push_back({3, r::attribute_format::f32, false});
    gbd.layout.attributes.push_back({2, r::attribute_format::f32, false});
    gbd.layout.attributes.push_back({4, r::attribute_format::f32, false});
    gbd.attribute_mapping.push_back({0, 0});
    gbd.attribute_mapping.push_back({0, 4 * 3});
    gbd.attribute_mapping.push_back({0, 4 * 3 + 4 * 2});
    gbd.buffer_mapping.push_back({0, 0, 4 * 9});


    r::buffer_handle buffers_to_binding[] = {buffer1};
    auto             geometry1 = gdevice->create_geometry(gbd, buffers_to_binding);

    while (app->is_runing()) {
        app->poll_events();

        glClearDepth(1.0);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        comlist->bind_pipeline(pipeline);

        comlist->bind_geometry(geometry1);

        auto binding = 0;
        glActiveTexture(GL_TEXTURE0 + binding);
        glBindTexture(GL_TEXTURE_2D, tex1.id);
        glBindSampler(binding, sampler1.id);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, tex2.id);
        glBindSampler(2, sampler1.id);

        // glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // glBindVertexArray(0);

        context->swap_buffers();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

#include <stdio.h>

#include "render_app_base.hpp"
#include "image_loader.hpp"
#include "resource_locator.hpp"

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/renderer/rhi/command_list.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/render_system.hpp>

namespace rhi = tavros::renderer::rhi;

const char* fullscreen_quad_vertex_shader_source = R"(
#version 420 core

const vec2 quadVerts[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);
const vec2 quadUVs[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);
out vec2 texCoord;

void main()
{
    vec2 pos = quadVerts[gl_VertexID];
    texCoord = quadUVs[gl_VertexID];
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

const char* fullscreen_quad_fragment_shader_source = R"(
#version 420 core

in vec2 texCoord;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uTex;

void main()
{
    vec3 color = texture(uTex, texCoord).rgb;
    FragColor = vec4(color, 1.0f);
}
)";

namespace rhi = tavros::renderer::rhi;

static tavros::core::logger logger("main");

[[noreturn]] void exit_fail()
{
    std::exit(-1);
}


class my_app : public app::render_app_base
{
public:
    my_app()
        : app::render_app_base("TavrosEngine")
        , m_res_locator("C:\\Users\\Piotr\\Desktop\\Tavros\\assets")
        , m_im_loader(&m_allocator)
    {
    }

    ~my_app() override
    {
    }


    rhi::buffer_handle create_stage_buffer(size_t size)
    {
        rhi::buffer_create_info info;
        info.size = size;
        info.usage = rhi::buffer_usage::stage;
        info.access = rhi::buffer_access::cpu_to_gpu;
        auto buffer = m_graphics_device->create_buffer(info);
        if (!buffer.is_valid()) {
            ::logger.fatal("Failed to create stage buffer.");
            exit_fail();
        }
        return buffer;
    }

    void init() override
    {
        m_graphics_device = rhi::graphics_device::create(rhi::render_backend_type::opengl);
        if (!m_graphics_device) {
            ::logger.fatal("Failed to create graphics_device.");
            exit_fail();
        }

        m_render_system = tavros::core::make_unique<tavros::renderer::render_system>(m_graphics_device.get());
        if (!m_render_system) {
            ::logger.fatal("Failed to create render_system.");
            exit_fail();
        }

        rhi::frame_composer_create_info main_composer_info;
        main_composer_info.width = 1;
        main_composer_info.height = 1;
        main_composer_info.buffer_count = 3;
        main_composer_info.vsync = true;
        main_composer_info.color_attachment_format = rhi::pixel_format::rgba8un;
        main_composer_info.depth_stencil_attachment_format = rhi::pixel_format::depth24_stencil8;

        auto main_composer_handle = m_graphics_device->create_frame_composer(main_composer_info, native_window_handle());
        if (!main_composer_handle.is_valid()) {
            ::logger.fatal("Failed to create main frame composer.");
            exit_fail();
        }

        m_composer = m_graphics_device->get_frame_composer_ptr(main_composer_handle);
        if (m_composer == nullptr) {
            ::logger.fatal("Failed to get main frame composer.");
            exit_fail();
        }

        auto fullscreen_quad_vertex_shader = m_graphics_device->create_shader({fullscreen_quad_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto fullscreen_quad_fragment_shader = m_graphics_device->create_shader({fullscreen_quad_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info main_pipeline_info;
        main_pipeline_info.shaders.push_back({rhi::shader_stage::vertex, "main"});
        main_pipeline_info.shaders.push_back({rhi::shader_stage::fragment, "main"});
        main_pipeline_info.depth_stencil.depth_test_enable = false;
        main_pipeline_info.depth_stencil.depth_write_enable = false;
        main_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
        main_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        main_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        main_pipeline_info.topology = rhi::primitive_topology::triangle_strip;
        main_pipeline_info.blend_states.push_back({false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});

        rhi::shader_handle fullscreen_quad_shaders[] = {fullscreen_quad_vertex_shader, fullscreen_quad_fragment_shader};
        m_main_pipeline = m_graphics_device->create_pipeline(main_pipeline_info, fullscreen_quad_shaders);

        m_stage_buffer = create_stage_buffer(1024 * 1024 * 16);

        auto tex_path = m_res_locator.resolve_texture("test.png").string();
        auto im_view = m_im_loader.load_image(tex_path);

        size_t tex_size = im_view.width * im_view.height * im_view.channels;
        auto*  dst = m_graphics_device->map_buffer(m_stage_buffer);
        memcpy(dst, im_view.data, tex_size);
        m_graphics_device->unmap_buffer(m_stage_buffer);

        rhi::texture_create_info tex_create_info;
        tex_create_info.type = rhi::texture_type::texture_2d;
        tex_create_info.format = rhi::pixel_format::rgba8un;
        tex_create_info.width = im_view.width;
        tex_create_info.height = im_view.height;
        tex_create_info.depth = 1;
        tex_create_info.usage = rhi::k_default_texture_usage;
        tex_create_info.mip_levels = 1;
        tex_create_info.array_layers = 1;
        tex_create_info.sample_count = 1;

        m_texture = m_graphics_device->create_texture(tex_create_info);
        if (!m_texture.is_valid()) {
            ::logger.fatal("Failed to create texture");
            exit_fail();
        }


        auto* cbuf = m_composer->create_command_list();
        cbuf->copy_buffer_to_texture(m_stage_buffer, m_texture, 0, tex_size, 0);
        m_composer->submit_command_list(cbuf);

        rhi::sampler_create_info sampler_info;
        sampler_info.filter.mipmap_filter = rhi::mipmap_filter_mode::off;
        sampler_info.filter.min_filter = rhi::filter_mode::linear;
        sampler_info.filter.mag_filter = rhi::filter_mode::linear;

        m_sampler = m_graphics_device->create_sampler(sampler_info);
        if (!m_sampler.is_valid()) {
            ::logger.fatal("Failed to create sampler");
            exit_fail();
        }

        rhi::render_pass_create_info main_render_pass;
        main_render_pass.color_attachments.push_back({rhi::pixel_format::rgba8un, 1, rhi::load_op::clear, rhi::store_op::dont_care, 0, {0.1f, 0.7f, 0.4f, 1.0f}});
        main_render_pass.depth_stencil_attachment = {rhi::pixel_format::depth24_stencil8, rhi::load_op::dont_care, rhi::store_op::dont_care, 1.0f, rhi::load_op::dont_care, rhi::store_op::dont_care, 0};
        m_main_pass = m_graphics_device->create_render_pass(main_render_pass);
        if (!m_main_pass.is_valid()) {
            ::logger.fatal("Failed to create render pass");
            exit_fail();
        }

        rhi::shader_binding_create_info shader_binding_info;
        shader_binding_info.texture_bindings.push_back({0, 0, 0});
        rhi::texture_handle textures_to_binding_main[] = {m_texture};
        rhi::sampler_handle samplers_to_binding_main[] = {m_sampler};
        m_shader_binding = m_graphics_device->create_shader_binding(shader_binding_info, textures_to_binding_main, samplers_to_binding_main, {});
        if (!m_shader_binding.is_valid()) {
            ::logger.fatal("Failed to create render pass");
            exit_fail();
        }
    }

    void shutdown() override
    {
        m_render_system = nullptr;
        m_graphics_device = nullptr;
    }

    void render(app::event_queue::queue_view events, float delta_time) override
    {
        bool need_resize = false;
        for (auto* it = events.begin; it != events.end; ++it) {
            if (it->type == app::event_type::window_resize) {
                m_current_size = tavros::math::ivec2(static_cast<int32>(it->vec_info.x), static_cast<int32>(it->vec_info.y));
                need_resize = true;
            }
        }

        if (need_resize) {
            m_composer->resize(m_current_size.width, m_current_size.height);
        }

        auto* cbuf = m_composer->create_command_list();
        m_composer->begin_frame();

        cbuf->begin_render_pass(m_main_pass, m_composer->backbuffer());
        cbuf->bind_pipeline(m_main_pipeline);
        cbuf->bind_shader_binding(m_shader_binding);
        cbuf->draw(4);
        cbuf->end_render_pass();

        m_composer->submit_command_list(cbuf);
        m_composer->end_frame();
        m_composer->present();


        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

private:
    tavros::core::mallocator                                  m_allocator;
    tavros::core::unique_ptr<rhi::graphics_device>            m_graphics_device;
    tavros::core::unique_ptr<tavros::renderer::render_system> m_render_system;
    rhi::frame_composer*                                      m_composer = nullptr;

    app::resource_locator m_res_locator;
    app::image_loader     m_im_loader;

    rhi::pipeline_handle       m_main_pipeline;
    rhi::render_pass_handle    m_main_pass;
    rhi::shader_binding_handle m_shader_binding;
    rhi::texture_handle        m_texture;
    rhi::buffer_handle         m_stage_buffer;
    rhi::sampler_handle        m_sampler;

    tavros::math::ivec2 m_current_size;
};

int main()
{
    tavros::core::logger::add_consumer([](auto lvl, auto tag, auto msg) {
        printf("%s\n", msg.data());
    });

    auto app = std::make_unique<my_app>();
    return app->run();
}
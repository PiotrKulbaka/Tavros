#include "render_app_base.hpp"
#include "built_in_meshes.hpp"
#include "fps_meter.hpp"

#include "scene_data.hpp"
#include "text_rendering.hpp"
#include "pipeline_presets.hpp"
#include "particle_system.hpp"

#include <tavros/renderer/shaders/shader_loader.hpp>
#include <tavros/renderer/shaders/shader_source_provider.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/exception.hpp>
#include <tavros/core/timer.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/camera/camera.hpp>
#include <tavros/renderer/debug_renderer.hpp>
#include <tavros/renderer/render_target.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>
#include <tavros/input/input_manager.hpp>
#include <tavros/assets/asset_manager.hpp>
#include <tavros/assets/image/image_view.hpp>
#include <tavros/ui/view.hpp>
#include <tavros/ui/button/button.hpp>
#include <tavros/ui/root_view.hpp>
#include <tavros/renderer/text/font/font_library.hpp>
#include <tavros/renderer/text/text_layouter.hpp>
#include <tavros/renderer/text/font/font_data_provider.hpp>
#include <tavros/system/application.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/memory/object_pool.hpp>
#include <tavros/assets/providers/filesystem_provider.hpp>

#include <cstdio>
#include <cstdlib>

namespace rhi = tavros::renderer::rhi;

static tavros::core::logger g_logger("main");

#define TAV_FATAL_IF(expr, msg)        \
    do {                               \
        if ((expr)) {                  \
            g_logger.fatal("{}", msg); \
            ::std::abort();            \
        }                              \
    } while (0)


constexpr size_t operator""_MiB(unsigned long long v) noexcept
{
    return v * 1024ull * 1024ull;
}
constexpr size_t operator""_KiB(unsigned long long v) noexcept
{
    return v * 1024ull;
}

rhi::pixel_format to_rhi_pixel_format(tavros::assets::image::pixel_format fmt) noexcept
{
    switch (fmt) {
    case tavros::assets::image::pixel_format::none:
        return rhi::pixel_format::none;
    case tavros::assets::image::pixel_format::r8:
        return rhi::pixel_format::r8un;
    case tavros::assets::image::pixel_format::rg8:
        return rhi::pixel_format::rg8un;
    case tavros::assets::image::pixel_format::rgb8:
        return rhi::pixel_format::rgb8un;
    case tavros::assets::image::pixel_format::rgba8:
        return rhi::pixel_format::rgba8un;
    default:
        TAV_UNREACHABLE();
    }
}

template<class Handle, class Resource>
class resource_manager_base
{
};

class resource_manager : tavros::core::noncopyable
{
public:
    resource_manager()
    {
        m_asset_manager = tavros::core::make_unique<tavros::assets::asset_manager>();
        m_asset_manager->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH, "", tavros::assets::asset_open_mode::read_only);
        m_asset_manager->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH, "file", tavros::assets::asset_open_mode::read_only);
        m_asset_manager->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH "/shaders", "", tavros::assets::asset_open_mode::read_only);
        m_asset_manager->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH "/shaders", "file", tavros::assets::asset_open_mode::read_only);
        m_asset_manager->mount<tavros::assets::filesystem_provider>(TAV_OUTPUT_PATH, "output", tavros::assets::asset_open_mode::write_only);
    }

    tavros::assets::asset_manager& asset_manager() noexcept
    {
        return *m_asset_manager.get();
    }

private:
    tavros::core::unique_ptr<tavros::assets::asset_manager> m_asset_manager;
};

// -------------------------------------------------------------------------
// Asset providers
// -------------------------------------------------------------------------

class filesystem_shader_provider : public tavros::renderer::shader_source_provider
{
public:
    explicit filesystem_shader_provider(
        tavros::core::shared_ptr<resource_manager> rm
    )
        : m_rm(std::move(rm))
    {
    }

    tavros::core::string load(tavros::core::string_view path) override
    {
        return m_rm->asset_manager().read_text(path);
    }

private:
    tavros::core::shared_ptr<resource_manager> m_rm;
};

class filesystem_font_provider : public tavros::renderer::font_data_provider
{
public:
    explicit filesystem_font_provider(
        tavros::core::shared_ptr<resource_manager> rm
    )
        : m_rm(std::move(rm))
    {
    }

    tavros::core::dynamic_buffer<uint8> load(tavros::core::string_view path) override
    {
        return m_rm->asset_manager().read_binary(path);
    }

private:
    tavros::core::shared_ptr<resource_manager> m_rm;
};

// -------------------------------------------------------------------------
// main_window
// -------------------------------------------------------------------------

class main_window : public app::render_app_base
{
public:
    main_window(tavros::core::string_view name, tavros::core::shared_ptr<resource_manager> rm)
        : app::render_app_base(name)
        , m_resource_manager(rm)
        , m_sl(tavros::core::make_unique<filesystem_shader_provider>(rm))
        , m_font_lib(tavros::core::make_unique<filesystem_font_provider>(rm))
    {
    }

    ~main_window() override = default;

    // ------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------

    void init() override
    {
        init_graphics();
        init_offscreen();
        init_pipelines();
        init_scene_resources();
        init_fonts();
        init_ui();
        init_particle_pipeline();

        show();
    }

    void shutdown() override
    {
        m_stream_draw = nullptr;
        m_drenderer.shutdown();
        m_root_view.shutdown();
        m_offscreen_rt->shutdown();
        m_graphics_device = nullptr;
    }

    void render(tavros::input::event_args_queue_view events, double delta_time) override
    {
        m_delta_time = static_cast<float>(m_delta_time);
        m_time = m_timer.elapsed_seconds();

        tavros::core::timer frame_timer;

        process_input(events, delta_time);
        m_fps_meter.tick(delta_time);
        update_frame_data();
        spawn_effects(delta_time);
        app::update_particles(m_particles, static_cast<float>(delta_time));

        m_stream_draw->begin_frame();
        m_drenderer.begin_frame(
            m_scene_data.ortho_projection,
            m_scene_data.view_projection
        );

        upload_frame_uniforms();
        build_fps_text();

        auto glyph_slice = m_stream_draw->slice<app::glyph_instance>(m_fps_text.size());
        auto glyph_count = app::fill_glyph_instances(m_fps_text, glyph_slice.data(), {16.0f, 16.0f});
        auto particle_slice = prepare_particle_slice();

        auto* cbuf = m_composer->create_command_queue();
        m_composer->begin_frame();

        record_offscreen_pass(cbuf, glyph_slice, glyph_count, particle_slice);

        if (m_capture_next_frame) {
            capture_frame(cbuf);
        }

        record_blit_to_backbuffer(cbuf);

        m_composer->submit_command_queue(cbuf);
        m_composer->end_frame();

        m_composer->wait_for_frame_complete();
        m_composer->present();

        if (m_capture_next_frame) {
            save_frame_capture();
            m_capture_next_frame = false;
        }

        m_input_manager.end_frame();
        ++m_frame_number;
    }

private:
    // ==================================================================
    // Init helpers
    // ==================================================================

    void init_graphics()
    {
        m_graphics_device = rhi::graphics_device::create(rhi::render_backend_type::opengl);
        TAV_FATAL_IF(!m_graphics_device, "Failed to create graphics device.");

        rhi::frame_composer_create_info info;
        info.width = 1;
        info.height = 1;
        info.buffer_count = 3;
        info.vsync = true;
        info.color_attachment_format = rhi::pixel_format::rgba8un;
        info.depth_stencil_attachment_format = rhi::pixel_format::depth32f_stencil8;
        info.native_handle = native_handle();

        auto handle = m_graphics_device->create_frame_composer(info);
        TAV_FATAL_IF(!handle, "Failed to create frame composer.");

        m_composer = m_graphics_device->get_frame_composer_ptr(handle);
        TAV_FATAL_IF(!m_composer, "Failed to get frame composer pointer.");

        m_fence = m_graphics_device->create_fence();
        TAV_FATAL_IF(!m_fence, "Failed to create fence.");

        m_stage_buffer = create_buffer(128_MiB, rhi::buffer_usage::stage, rhi::buffer_access::cpu_to_gpu);
        m_stage_upload_buffer = create_buffer(64_MiB, rhi::buffer_usage::stage, rhi::buffer_access::gpu_to_cpu);
        m_uniform_buffer = create_buffer(1_MiB, rhi::buffer_usage::storage, rhi::buffer_access::gpu_only);

        m_stream_draw = tavros::core::make_unique<tavros::renderer::gpu_stream_buffer>(
            m_graphics_device.get(), 128_MiB, rhi::buffer_usage::vertex
        );

        m_camera.set_orientation({1.0f, 1.0f, -0.25f}, {0.0f, 0.0f, 1.0f});
        m_camera.set_position({-8.0f, -8.0f, 5.0f});
    }

    void init_offscreen()
    {
        m_offscreen_rt = tavros::core::make_unique<tavros::renderer::render_target>(
            rhi::pixel_format::rgba8un,
            rhi::pixel_format::depth32f
        );
        m_offscreen_rt->init(m_graphics_device.get());

        rhi::render_pass_create_info rp;
        rp.color_attachments.push_back({rhi::pixel_format::rgba8un, rhi::load_op::clear, rhi::store_op::dont_care, {}, {0.2f, 0.2f, 0.25f, 1.0f}});
        rp.depth_stencil_attachment.format = rhi::pixel_format::depth32f_stencil8;
        rp.depth_stencil_attachment.depth_load = rhi::load_op::clear;
        rp.depth_stencil_attachment.depth_store = rhi::store_op::store;
        rp.depth_stencil_attachment.depth_clear_value = 1.0f;
        rp.depth_stencil_attachment.stencil_load = rhi::load_op::clear;
        rp.depth_stencil_attachment.stencil_store = rhi::store_op::dont_care;
        rp.depth_stencil_attachment.stencil_clear_value = 0;

        m_main_pass = m_graphics_device->create_render_pass(rp);
        TAV_FATAL_IF(!m_main_pass, "Failed to create main render pass.");
    }

    void init_pipelines()
    {
        m_fullscreen_quad_pipeline = build_fullscreen_quad_pipeline();
        m_mesh_rendering_pipeline = build_mesh_pipeline();
        m_world_grid_pipeline = build_world_grid_pipeline();
        m_sdf_font_pipeline = build_sdf_font_pipeline();
    }

    void init_scene_resources()
    {
        // Cube texture
        auto im = load_image("textures/cube_test.png");
        upload_to_stage(im.data(), im.size_bytes());

        rhi::texture_create_info tex_info;
        tex_info.type = rhi::texture_type::texture_2d;
        tex_info.format = to_rhi_pixel_format(im.format());
        tex_info.width = im.width();
        tex_info.height = im.height();
        tex_info.depth = 1;
        tex_info.usage = rhi::k_default_texture_usage;
        tex_info.mip_levels = tavros::math::mip_levels(im.width(), im.height());
        tex_info.array_layers = 1;
        tex_info.sample_count = 1;

        m_cube_texture = m_graphics_device->create_texture(tex_info);
        TAV_FATAL_IF(!m_cube_texture, "Failed to create cube texture.");

        rhi::texture_copy_region region;
        region.width = im.width();
        region.height = im.height();
        region.buffer_row_length = im.stride() / im.components();

        auto* cbuf = m_composer->create_command_queue();
        cbuf->copy_buffer_to_texture(m_stage_buffer, m_cube_texture, region);
        cbuf->signal_fence(m_fence);
        m_composer->submit_command_queue(cbuf);

        // Sampler
        rhi::sampler_create_info si;
        si.filter.mipmap_filter = rhi::mipmap_filter_mode::off;
        si.filter.min_filter = rhi::filter_mode::linear;
        si.filter.mag_filter = rhi::filter_mode::linear;
        m_sampler = m_graphics_device->create_sampler(si);
        TAV_FATAL_IF(!m_sampler, "Failed to create sampler.");

        // Mesh buffers
        m_mesh_vertices_buffer = create_buffer(16_MiB, rhi::buffer_usage::vertex, rhi::buffer_access::cpu_to_gpu);
        m_mesh_indices_buffer = create_buffer(128_KiB, rhi::buffer_usage::index, rhi::buffer_access::cpu_to_gpu);

        upload_to_mapped(m_mesh_vertices_buffer, app::cube_vertices, sizeof(app::cube_vertices));
        upload_to_mapped(m_mesh_indices_buffer, app::cube_indices, sizeof(app::cube_indices));

        // Shader bindings
        m_mesh_shader_binding = create_texture_binding(m_cube_texture, m_sampler);
        m_scene_binding = create_buffer_binding(m_uniform_buffer, sizeof(app::scene_data));

        m_graphics_device->wait_for_fence(m_fence);
    }

    void init_fonts()
    {
        m_font_lib.load("fonts/Consola-Mono.ttf", "Consola-Mono");
        m_font_lib.load("fonts/DroidSans.ttf", "DroidSans");
        m_font_lib.load("fonts/HomeVideo.ttf", "HomeVideo");
        m_font_lib.load("fonts/NotoSans-Regular.ttf", "NotoSans-Regular");
        m_font_lib.load("fonts/Roboto-Medium.ttf", "Roboto-Medium");

        m_font_shader_binding = bake_and_upload_font_atlas();
    }

    void init_ui()
    {
        m_drenderer.init(m_graphics_device.get());
        m_root_view.init(m_graphics_device.get());

        // Build view hierarchy
        for (auto* v : {&m_view1, &m_view2, &m_view3, &m_view4, &m_view5, (tavros::ui::view*) &m_btn0}) {
            m_root_view.root().insert_last_child(v);
        }
        for (auto* v : {&m_view6, &m_view7, &m_view8, &m_view9, (tavros::ui::view*) &m_btn1}) {
            m_view5.insert_last_child(v);
        }
        m_view9.insert_last_child(&m_view10);
        m_view9.insert_last_child(&m_view11);

        m_view5.set_enabled(false);

        // Layout
        m_btn0.set_position({1400, 100});
        m_btn0.set_size({240.0f, 80.0f});
        m_btn0.set_text("Button0");
        m_btn1.set_position({300, 100});
        m_btn1.set_size({240.0f, 80.0f});
        m_btn1.set_text("Button1");

        m_view1.set_position({100, 100});
        m_view1.set_size({100, 100});
        m_view2.set_position({100, 300});
        m_view2.set_size({100, 100});
        m_view3.set_position({300, 100});
        m_view3.set_size({150, 150});
        m_view4.set_position({300, 300});
        m_view4.set_size({200, 100});

        m_view5.set_position({1250, 100});
        m_view5.set_size({600, 600});
        m_view5.set_padding({10.0f, 20.0f, 30.0f, 40.0f});

        m_view6.set_position({10, 10});
        m_view6.set_size({100, 100});
        m_view7.set_position({250, 50});
        m_view7.set_size({100, 150});
        m_view7.set_padding({10, 10, 10, 10});
        m_view8.set_position({50, 250});
        m_view8.set_size({100, 150});
        m_view9.set_position({250, 250});
        m_view9.set_size({250, 300});

        m_view10.set_position({50, 50});
        m_view10.set_size({100, 50});
        m_view11.set_position({50, 150});
        m_view11.set_size({150, 100});
    }

    void init_particle_pipeline()
    {
        using namespace app::pipeline_builder;

        auto vert = load_and_create_shader("tavros/shaders/particle.vert", rhi::shader_stage::vertex);
        auto frag = load_and_create_shader("tavros/shaders/particle.frag", rhi::shader_stage::fragment);

        rhi::pipeline_create_info info;
        info.bindings.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0, sizeof(app::particle_instance), offsetof(app::particle_instance, position), 1});
        info.bindings.push_back({rhi::attribute_type::scalar, rhi::attribute_format::f32, false, 1, sizeof(app::particle_instance), offsetof(app::particle_instance, size), 1});
        info.bindings.push_back({rhi::attribute_type::vec4, rhi::attribute_format::f32, false, 2, sizeof(app::particle_instance), offsetof(app::particle_instance, color), 1});
        info.bindings.push_back({rhi::attribute_type::scalar, rhi::attribute_format::f32, false, 3, sizeof(app::particle_instance), offsetof(app::particle_instance, rotation), 1});
        info.shaders.push_back(vert);
        info.shaders.push_back(frag);
        info.depth_stencil = depth_off();
        info.rasterizer = cull_off();
        info.topology = rhi::primitive_topology::triangle_strip;
        info.blend_states.push_back(additive_blend());
        info.multisample = no_msaa();

        m_particle_pipeline = m_graphics_device->create_pipeline(info);
        TAV_FATAL_IF(!m_particle_pipeline, "Failed to create particle pipeline.");

        m_graphics_device->destroy_shader(vert);
        m_graphics_device->destroy_shader(frag);
    }

    // ==================================================================
    // Per-frame helpers
    // ==================================================================

    void process_input(tavros::input::event_args_queue_view events, double delta_time)
    {
        m_root_view.process_ui_events(events);

        m_input_manager.begin_frame(tavros::system::application::instance().highp_time_us());
        m_input_manager.process_events(events);

        handle_key_shortcuts();
        handle_window_resize();
        update_camera(delta_time);
    }

    void handle_key_shortcuts()
    {
        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_F1)) {
            m_current_buffer_output = 0;
        }
        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_F2)) {
            m_current_buffer_output = 1;
        }
        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_F10)) {
            m_capture_next_frame = true;
        }

        if (m_input_manager.is_key_pressed(tavros::input::keyboard_key::k_minus)) {
            m_font_height -= 0.2f;
        }
        if (m_input_manager.is_key_pressed(tavros::input::keyboard_key::k_equal)) {
            m_font_height += 0.2f;
        }
    }

    void handle_window_resize()
    {
        if (!m_input_manager.is_window_resized()) {
            return;
        }

        const auto sz = m_input_manager.get_window_size();
        if (sz.width == 0 || sz.height == 0) {
            return;
        }

        m_offscreen_rt->recreate(
            static_cast<uint32>(sz.width),
            static_cast<uint32>(sz.height), 32
        );

        recreate_offscreen_bindings();

        m_composer->resize(sz.width, sz.height);

        constexpr float k_fov_y = 60.0f * 3.14159265358979f / 180.0f;
        m_camera.set_perspective(
            k_fov_y,
            static_cast<float>(sz.width) / static_cast<float>(sz.height),
            0.1f, 1000.0f
        );
    }

    void update_camera(double delta_time)
    {
        const auto factor = [&](tavros::input::keyboard_key key) {
            return static_cast<float>(m_input_manager.key_hold_factor(key));
        };

        tavros::math::vec3 move =
            m_camera.forward() * (factor(tavros::input::keyboard_key::k_W) - factor(tavros::input::keyboard_key::k_S))
            + m_camera.right() * (factor(tavros::input::keyboard_key::k_A) - factor(tavros::input::keyboard_key::k_D))
            + m_camera.up() * (factor(tavros::input::keyboard_key::k_space) - factor(tavros::input::keyboard_key::k_C));

        const float len = tavros::math::length(move);
        if (len > 1.0f) {
            move /= len;
        }

        const bool  shift = m_input_manager.is_key_pressed(tavros::input::keyboard_key::k_lshift);
        const bool  ctrl = m_input_manager.is_key_pressed(tavros::input::keyboard_key::k_lcontrol);
        const float speed = static_cast<float>(delta_time) * (shift ? (ctrl ? 100.0f : 10.0f) : 2.0f);
        m_camera.move(move * speed);

        const auto mouse_delta = m_input_manager.get_smooth_mouse_delta();
        if (tavros::math::squared_length(mouse_delta) > 0.0f) {
            apply_camera_rotation(mouse_delta);
        }
    }

    void apply_camera_rotation(tavros::math::vec2 mouse_delta)
    {
        constexpr float k_sensitivity = 0.5f;
        const auto      d = mouse_delta * k_sensitivity;

        const auto world_up = m_camera.world_up();
        const auto q_yaw = tavros::math::quat::from_axis_angle(world_up, d.x);
        const auto q_pitch = tavros::math::quat::from_axis_angle(m_camera.right(), -d.y);
        const auto rotation = tavros::math::normalize(q_yaw * q_pitch);

        const auto candidate_forward = tavros::math::normalize(rotation * m_camera.forward());
        const auto horizontal_fwd = tavros::math::normalize(tavros::math::cross(m_camera.right(), world_up));

        if (tavros::math::dot(candidate_forward, horizontal_fwd) < 0.000001f) {
            const float sign = (tavros::math::dot(m_camera.forward(), world_up) > 0) ? 1.0f : -1.0f;
            m_camera.set_orientation(q_yaw * (horizontal_fwd * 0.000001f + world_up * sign), world_up);
            m_camera.rotate(q_yaw);
        } else {
            m_camera.set_orientation(candidate_forward, world_up);
        }
    }

    void update_frame_data()
    {
        const float w = static_cast<float>(m_input_manager.get_window_size().width);
        const float h = static_cast<float>(m_input_manager.get_window_size().height);

        m_scene_data.view = m_camera.get_view_matrix();
        m_scene_data.projection = m_camera.get_projection_matrix();
        m_scene_data.view_projection = m_camera.get_view_projection_matrix();
        m_scene_data.inverse_view = tavros::math::inverse(m_camera.get_view_matrix());
        m_scene_data.inverse_projection = tavros::math::inverse(m_camera.get_projection_matrix());
        m_scene_data.ortho_projection = tavros::math::mat4::ortho(0.0f, w, h, 0.0f, 1.0f, -1.0f);
        m_scene_data.camera_position = m_camera.position();
        m_scene_data.near_plane = m_camera.near_plane();
        m_scene_data.far_plane = m_camera.far_plane();
        m_scene_data.aspect_ratio = m_camera.aspect();
        m_scene_data.fov_y = m_camera.fov_y();
        m_scene_data._pad0 = 0.0f;
        m_scene_data.frame_size = {w, h};
        m_scene_data.time = m_time;
        m_scene_data.delta_time = m_delta_time;
        m_scene_data.frame_index = static_cast<uint32>(m_frame_number);
        m_scene_data._pad1 = 0.0f;
    }

    void spawn_effects(double delta_time)
    {
        auto        rng = [&](float lo, float hi) { return rand_range(lo, hi); };
        const float time = static_cast<float>(m_frame_number) * 0.016f;

        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_E)) {
            app::particle_spawner::explosion(m_particles, {10.0f, 5.0f, 0.0f}, 150, rng);
        }
        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_3)) {
            app::particle_spawner::star_burst(m_particles, {0.0f, 2.0f, 0.0f}, 160, rng);
        }

        app::particle_spawner::fountain(m_particles, {10.0f, 10.0f, 0.0f}, 10, rng);
        app::particle_spawner::vortex(m_particles, {5.0f, 10.0f, 0.0f}, time, 30, rng);
        app::particle_spawner::lightning(m_particles, {0.0f, 10.0f, 0.0f}, 30.0f, rng);
        app::particle_spawner::portal(m_particles, {-5.0f, 10.0f, 0.0f}, 1.0f, static_cast<float>(delta_time), 40, rng);
        app::particle_spawner::smoke(m_particles, {-10.0f, -10.0f, 0.0f}, 3, rng);
        app::particle_spawner::snowfall(m_particles, {0.0f, 0.0f, 0.0f}, 60.0f, 60.0f, 10.0f, 50, rng);

        m_meteor_pos += m_meteor_vel * static_cast<float>(delta_time);
        app::particle_spawner::meteor_trail(m_particles, m_meteor_pos, m_meteor_vel, 20, rng);
    }

    void upload_frame_uniforms()
    {
        auto map = m_graphics_device->map_buffer(m_stage_buffer, 0, sizeof(m_scene_data));
        map.copy_from(&m_scene_data, sizeof(m_scene_data));
        m_graphics_device->unmap_buffer(m_stage_buffer);
    }

    void build_fps_text()
    {
        const auto*     fnt = m_font_lib.fonts()[0].font.get();
        constexpr float sz = 36.0f;

        const tavros::math::vec4 col_label = {0.8f, 0.2f, 1.0f, 1.0f};
        const tavros::math::vec4 col_value = {1.0f, 1.0f, 1.0f, 1.0f};
        const tavros::math::vec4 col_none = {1.0f, 1.0f, 1.0f, 0.0f};

        m_fps_text.clear();

        auto label = [&](const char* text) {
            app::append_colored_text(m_fps_text, text, fnt, sz, col_label, col_none);
        };
        auto value = [&](auto v) {
            app::append_colored_text(m_fps_text, std::to_string(v), fnt, sz, col_value, col_none);
        };

        label("Frame:   ");
        value(m_frame_number);
        label("\nAvg FPS: ");
        value(m_fps_meter.average_fps());
        label("\nMed FPS: ");
        value(m_fps_meter.median_fps());
        label("\nDelta ms: ");
        value(m_delta_time);
        label("\nParticles: ");
        value(m_particles.size());

        tavros::renderer::text_layouter::layout(m_fps_text);
    }

    struct particle_draw_slice
    {
        tavros::renderer::gpu_buffer_view<app::particle_instance> slice{};
        size_t                                                    count = 0;
    };

    particle_draw_slice prepare_particle_slice()
    {
        particle_draw_slice result;
        if (m_particles.size() > 0) {
            result.slice = m_stream_draw->slice<app::particle_instance>(m_particles.size());
            result.count = app::fill_particle_instances(m_particles, result.slice.data());
        }
        return result;
    }

    void record_offscreen_pass(
        rhi::command_queue*                                           cbuf,
        const tavros::renderer::gpu_buffer_view<app::glyph_instance>& glyph_slice,
        size_t                                                        glyph_count,
        const particle_draw_slice&                                    pslice
    )
    {
        cbuf->copy_buffer(m_stage_buffer, m_uniform_buffer, sizeof(m_scene_data));
        cbuf->begin_render_pass(m_offscreen_rt->render_pass(), m_offscreen_rt->framebuffer());

        draw_cube(cbuf);
        draw_world_grid(cbuf);
        draw_hud_text(cbuf, glyph_slice, glyph_count);
        draw_particles(cbuf, pslice);

        m_drenderer.update();
        m_drenderer.render(cbuf);
        m_drenderer.end_frame();

        m_root_view.on_frame(cbuf, 0.0f);

        cbuf->end_render_pass();
    }

    void draw_cube(rhi::command_queue* cbuf)
    {
        cbuf->bind_pipeline(m_mesh_rendering_pipeline);
        rhi::bind_buffer_info bufs[] = {
            {m_mesh_vertices_buffer, 0},
            {m_mesh_vertices_buffer, 0},
            {m_mesh_vertices_buffer, 0}
        };
        cbuf->bind_vertex_buffers(bufs);
        cbuf->bind_index_buffer(m_mesh_indices_buffer, rhi::index_buffer_format::u32);
        cbuf->bind_shader_binding(m_scene_binding);
        cbuf->bind_shader_binding(m_mesh_shader_binding);
        cbuf->draw_indexed(6 * 6);
    }

    void draw_world_grid(rhi::command_queue* cbuf)
    {
        cbuf->bind_pipeline(m_world_grid_pipeline);
        cbuf->bind_shader_binding(m_scene_binding);
        cbuf->draw(4);
    }

    void draw_hud_text(rhi::command_queue* cbuf, const tavros::renderer::gpu_buffer_view<app::glyph_instance>& slice, size_t count)
    {
        cbuf->bind_pipeline(m_sdf_font_pipeline);
        cbuf->bind_shader_binding(m_font_shader_binding);
        cbuf->bind_shader_binding(m_scene_binding);

        rhi::bind_buffer_info bufs[] = {
            {slice.gpu_buffer(), slice.offset_bytes()},
            {slice.gpu_buffer(), slice.offset_bytes()},
            {slice.gpu_buffer(), slice.offset_bytes()},
            {slice.gpu_buffer(), slice.offset_bytes()}
        };
        cbuf->bind_vertex_buffers(bufs);

        cbuf->set_scissor({0, 0, static_cast<int32>(m_composer->width()), static_cast<int32>(m_composer->height())});
        cbuf->draw(4, 0, static_cast<uint32>(count), 0);
    }

    void draw_particles(rhi::command_queue* cbuf, const particle_draw_slice& pslice)
    {
        if (pslice.count == 0) {
            return;
        }

        cbuf->bind_pipeline(m_particle_pipeline);
        cbuf->bind_shader_binding(m_scene_binding);

        rhi::bind_buffer_info bufs[] = {
            {pslice.slice.gpu_buffer(), pslice.slice.offset_bytes()},
            {pslice.slice.gpu_buffer(), pslice.slice.offset_bytes()},
            {pslice.slice.gpu_buffer(), pslice.slice.offset_bytes()},
            {pslice.slice.gpu_buffer(), pslice.slice.offset_bytes()}
        };
        cbuf->bind_vertex_buffers(bufs);
        cbuf->draw(4, 0, static_cast<uint32>(pslice.count), 0);
    }

    void record_blit_to_backbuffer(rhi::command_queue* cbuf)
    {
        cbuf->begin_render_pass(m_main_pass, m_composer->backbuffer());
        cbuf->bind_pipeline(m_fullscreen_quad_pipeline);
        cbuf->bind_shader_binding(
            m_current_buffer_output == 0 ? m_color_shader_binding : m_depth_stencil_shader_binding
        );
        cbuf->draw(4);
        cbuf->end_render_pass();
    }

    void capture_frame(rhi::command_queue* cbuf)
    {
        const int w = m_input_manager.get_window_size().width;
        const int h = m_input_manager.get_window_size().height;

        rhi::texture_copy_region rgn;
        rgn.width = w;
        rgn.height = h;
        cbuf->copy_texture_to_buffer(m_offscreen_rt->get_color_attachment(0), m_stage_upload_buffer, rgn);

        rgn.buffer_offset = static_cast<size_t>(w * h * 4);
        cbuf->copy_texture_to_buffer(m_offscreen_rt->get_depth_stencil_attachment(), m_stage_upload_buffer, rgn);
    }

    void save_frame_capture()
    {
        const int w = m_input_manager.get_window_size().width;
        const int h = m_input_manager.get_window_size().height;

        auto         map = m_graphics_device->map_buffer(m_stage_upload_buffer);
        const size_t plane_size = static_cast<size_t>(w * h);

        // Color
        tavros::assets::image_view color_im({map.data(), plane_size * 4}, w, h, tavros::assets::image::pixel_format::rgba8);
        save_image(color_im, "output://color.png", true);

        // Depth (float -> u8)
        tavros::core::dynamic_buffer<uint8> depth_buf(plane_size);

        const float* src = reinterpret_cast<const float*>(map.data() + plane_size * 4);
        for (size_t i = 0; i < plane_size; ++i) {
            depth_buf[i] = static_cast<uint8>(src[i] * 255.0f);
        }

        tavros::assets::image_view depth_im(depth_buf, w, h, tavros::assets::image::pixel_format::r8);
        save_image(depth_im, "output://depth.png", true);

        m_graphics_device->unmap_buffer(m_stage_upload_buffer);
    }

    // ==================================================================
    // Pipeline builders
    // ==================================================================

    rhi::pipeline_handle build_fullscreen_quad_pipeline()
    {
        using namespace app::pipeline_builder;
        auto vert = load_and_create_shader("tavros/shaders/fullscreen_quad.vert", rhi::shader_stage::vertex);
        auto frag = load_and_create_shader("tavros/shaders/fullscreen_quad.frag", rhi::shader_stage::fragment);

        rhi::pipeline_create_info info;
        info.shaders.push_back(vert);
        info.shaders.push_back(frag);
        info.depth_stencil = depth_off();
        info.rasterizer = cull_off();
        info.topology = rhi::primitive_topology::triangle_strip;
        info.blend_states.push_back(alpha_blend());

        auto h = m_graphics_device->create_pipeline(info);
        TAV_FATAL_IF(!h, "Failed to create fullscreen quad pipeline.");
        m_graphics_device->destroy_shader(vert);
        m_graphics_device->destroy_shader(frag);
        return h;
    }

    rhi::pipeline_handle build_mesh_pipeline()
    {
        using namespace app::pipeline_builder;
        auto vert = load_and_create_shader("tavros/shaders/cube.vert", rhi::shader_stage::vertex);
        auto frag = load_and_create_shader("tavros/shaders/cube.frag", rhi::shader_stage::fragment);

        rhi::pipeline_create_info info;
        info.bindings.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0, sizeof(app::vertex_type), offsetof(app::vertex_type, pos), 0});
        info.bindings.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 1, sizeof(app::vertex_type), offsetof(app::vertex_type, normal), 0});
        info.bindings.push_back({rhi::attribute_type::vec2, rhi::attribute_format::f32, false, 2, sizeof(app::vertex_type), offsetof(app::vertex_type, uv), 0});
        info.shaders.push_back(vert);
        info.shaders.push_back(frag);
        info.depth_stencil = depth_test_rw();
        info.rasterizer = cull_back_ccw();
        info.topology = rhi::primitive_topology::triangles;
        info.blend_states.push_back(no_blend());
        info.multisample = no_msaa();

        auto h = m_graphics_device->create_pipeline(info);
        TAV_FATAL_IF(!h, "Failed to create mesh pipeline.");
        m_graphics_device->destroy_shader(vert);
        m_graphics_device->destroy_shader(frag);
        return h;
    }

    rhi::pipeline_handle build_world_grid_pipeline()
    {
        using namespace app::pipeline_builder;
        auto vert = load_and_create_shader("tavros/shaders/world_grid.vert", rhi::shader_stage::vertex);
        auto frag = load_and_create_shader("tavros/shaders/world_grid.frag", rhi::shader_stage::fragment);

        rhi::pipeline_create_info info;
        info.shaders.push_back(vert);
        info.shaders.push_back(frag);
        info.depth_stencil = depth_test_ro();
        info.rasterizer = cull_off();
        info.topology = rhi::primitive_topology::triangle_strip;
        info.blend_states.push_back(alpha_blend());
        info.multisample = no_msaa();

        auto h = m_graphics_device->create_pipeline(info);
        TAV_FATAL_IF(!h, "Failed to create world grid pipeline.");
        m_graphics_device->destroy_shader(vert);
        m_graphics_device->destroy_shader(frag);
        return h;
    }

    rhi::pipeline_handle build_sdf_font_pipeline()
    {
        using namespace app::pipeline_builder;
        auto vert = load_and_create_shader("tavros/shaders/sdf_font.vert", rhi::shader_stage::vertex);
        auto frag = load_and_create_shader("tavros/shaders/sdf_font.frag", rhi::shader_stage::fragment);

        rhi::pipeline_create_info info;
        info.bindings.push_back({rhi::attribute_type::mat3x2, rhi::attribute_format::f32, false, 0, sizeof(app::glyph_instance), offsetof(app::glyph_instance, mat), 1});
        info.bindings.push_back({rhi::attribute_type::vec2, rhi::attribute_format::u32, false, 3, sizeof(app::glyph_instance), offsetof(app::glyph_instance, rect), 1});
        info.bindings.push_back({rhi::attribute_type::scalar, rhi::attribute_format::u32, false, 4, sizeof(app::glyph_instance), offsetof(app::glyph_instance, fill_color), 1});
        info.bindings.push_back({rhi::attribute_type::scalar, rhi::attribute_format::u32, false, 5, sizeof(app::glyph_instance), offsetof(app::glyph_instance, outline_color), 1});
        info.shaders.push_back(vert);
        info.shaders.push_back(frag);
        info.depth_stencil = depth_off();
        info.rasterizer = cull_off_scissor();
        info.topology = rhi::primitive_topology::triangle_strip;
        info.blend_states.push_back(alpha_blend());
        info.multisample = no_msaa();

        auto h = m_graphics_device->create_pipeline(info);
        TAV_FATAL_IF(!h, "Failed to create SDF font pipeline.");
        m_graphics_device->destroy_shader(vert);
        m_graphics_device->destroy_shader(frag);
        return h;
    }

    // ==================================================================
    // Resource helpers
    // ==================================================================

    rhi::buffer_handle create_buffer(
        size_t             size,
        rhi::buffer_usage  usage,
        rhi::buffer_access access
    )
    {
        rhi::buffer_create_info info{size, usage, access};
        auto                    h = m_graphics_device->create_buffer(info);
        TAV_FATAL_IF(!h, "Failed to create buffer (size={})", size);
        return h;
    }

    void upload_to_stage(const void* data, size_t size)
    {
        auto map = m_graphics_device->map_buffer(m_stage_buffer);
        map.copy_from(data, size);
        m_graphics_device->unmap_buffer(m_stage_buffer);
    }

    void upload_to_mapped(rhi::buffer_handle dst, const void* data, size_t size)
    {
        auto map = m_graphics_device->map_buffer(dst);
        map.copy_from(data, size);
        m_graphics_device->unmap_buffer(dst);
    }

    rhi::shader_handle load_and_create_shader(
        tavros::core::string_view path,
        rhi::shader_stage         stage
    )
    {
        const auto src = m_sl.load(path, {tavros::renderer::shader_language::glsl_460, ""});
        return m_graphics_device->create_shader({src, stage, "main"});
    }

    rhi::shader_binding_handle create_texture_binding(
        rhi::texture_handle texture,
        rhi::sampler_handle sampler,
        uint32              slot = 0
    )
    {
        rhi::shader_binding_create_info info;
        info.texture_bindings.push_back({texture, sampler, slot});
        auto h = m_graphics_device->create_shader_binding(info);
        TAV_FATAL_IF(!h, "Failed to create texture shader binding.");
        return h;
    }

    rhi::shader_binding_handle create_buffer_binding(
        rhi::buffer_handle buffer,
        size_t             size,
        uint32             slot = 0,
        size_t             offset = 0
    )
    {
        rhi::shader_binding_create_info info;
        info.buffer_bindings.push_back({buffer, static_cast<uint32>(offset), static_cast<uint32>(size), slot});
        auto h = m_graphics_device->create_shader_binding(info);
        TAV_FATAL_IF(!h, "Failed to create buffer shader binding.");
        return h;
    }

    tavros::assets::image load_image(tavros::core::string_view path, bool y_flip = false)
    {
        tavros::assets::image im;
        try {
            auto data = m_resource_manager->asset_manager().read_binary(path);
            return tavros::assets::image::decode(data, y_flip);
        } catch (const tavros::core::file_error& e) {
            g_logger.error("Failed to open image '{}'", e.path());
        }
        return {};
    }

    void save_image(tavros::assets::image_view im, tavros::core::string_view path, bool y_flip = false)
    {
        if (!im.valid()) {
            g_logger.error("Failed to save invalid image {}", path);
            return;
        }
        try {
            auto data = tavros::assets::image::encode(im, y_flip);
            m_resource_manager->asset_manager().open(path, tavros::assets::asset_open_mode::write_only)->write(data);
        } catch (const tavros::core::file_error& e) {
            g_logger.error("Failed to save image '{}'", e.path());
        }
    }

    rhi::shader_binding_handle bake_and_upload_font_atlas()
    {
        auto atlas = m_font_lib.invalidate_old_and_bake_new_atlas(96.0f, 8.0f);

        const size_t tex_size = atlas.width * atlas.height;
        upload_to_stage(atlas.pixels.data(), tex_size);

        rhi::texture_create_info ti;
        ti.type = rhi::texture_type::texture_2d;
        ti.format = rhi::pixel_format::r8un;
        ti.width = atlas.width;
        ti.height = atlas.height;
        ti.depth = 1;
        ti.usage = rhi::k_default_texture_usage;
        ti.mip_levels = 1;
        ti.array_layers = 1;
        ti.sample_count = 1;

        auto tex = m_graphics_device->create_texture(ti);
        TAV_FATAL_IF(!tex, "Failed to create font atlas texture.");

        rhi::sampler_create_info si;
        si.filter.mipmap_filter = rhi::mipmap_filter_mode::off;
        si.filter.min_filter = rhi::filter_mode::linear;
        si.filter.mag_filter = rhi::filter_mode::linear;
        auto smp = m_graphics_device->create_sampler(si);
        TAV_FATAL_IF(!smp, "Failed to create font atlas sampler.");

        rhi::texture_copy_region rgn;
        rgn.width = atlas.width;
        rgn.height = atlas.height;

        auto* cbuf = m_composer->create_command_queue();
        cbuf->wait_for_fence(m_fence);
        cbuf->copy_buffer_to_texture(m_stage_buffer, tex, rgn);
        cbuf->signal_fence(m_fence);
        m_composer->submit_command_queue(cbuf);

        return create_texture_binding(tex, smp);
    }

    void recreate_offscreen_bindings()
    {
        auto destroy = [&](rhi::shader_binding_handle& h) {
            if (h) {
                m_graphics_device->destroy_shader_binding(h);
                h = {};
            }
        };
        destroy(m_color_shader_binding);
        destroy(m_depth_stencil_shader_binding);

        m_color_shader_binding = create_texture_binding(m_offscreen_rt->get_color_attachment(0), m_sampler);
        m_depth_stencil_shader_binding = create_texture_binding(m_offscreen_rt->get_depth_stencil_attachment(), m_sampler);
    }

    float rand_range(float lo, float hi) noexcept
    {
        return lo + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * (hi - lo);
    }

    // ==================================================================
    // Members
    // ==================================================================

    // -- Core --
    tavros::core::shared_ptr<resource_manager> m_resource_manager;
    tavros::renderer::shader_loader            m_sl;

    // -- Graphics device --
    tavros::core::unique_ptr<rhi::graphics_device> m_graphics_device;
    rhi::frame_composer*                           m_composer = nullptr;
    rhi::fence_handle                              m_fence;

    // -- Offscreen RT --
    tavros::core::unique_ptr<tavros::renderer::render_target> m_offscreen_rt;

    // -- Pipelines --
    rhi::pipeline_handle m_fullscreen_quad_pipeline;
    rhi::pipeline_handle m_mesh_rendering_pipeline;
    rhi::pipeline_handle m_world_grid_pipeline;
    rhi::pipeline_handle m_sdf_font_pipeline;
    rhi::pipeline_handle m_particle_pipeline;

    // -- Render passes --
    rhi::render_pass_handle m_main_pass;

    // -- Shader bindings --
    rhi::shader_binding_handle m_mesh_shader_binding;
    rhi::shader_binding_handle m_scene_binding;
    rhi::shader_binding_handle m_color_shader_binding;
    rhi::shader_binding_handle m_depth_stencil_shader_binding;
    rhi::shader_binding_handle m_font_shader_binding;

    // -- GPU resources --
    rhi::texture_handle m_cube_texture;
    rhi::sampler_handle m_sampler;
    rhi::buffer_handle  m_stage_buffer;
    rhi::buffer_handle  m_stage_upload_buffer;
    rhi::buffer_handle  m_mesh_vertices_buffer;
    rhi::buffer_handle  m_mesh_indices_buffer;
    rhi::buffer_handle  m_uniform_buffer;

    tavros::core::unique_ptr<tavros::renderer::gpu_stream_buffer> m_stream_draw;

    // -- Scene --
    tavros::renderer::camera         m_camera;
    app::scene_data                  m_scene_data;
    tavros::renderer::debug_renderer m_drenderer;

    // -- Text / fonts --
    tavros::renderer::font_library m_font_lib;
    app::text_archetype            m_fps_text;
    float                          m_font_height = 100.0f;

    // -- UI --
    tavros::ui::root_view m_root_view;
    tavros::ui::view      m_view1, m_view2, m_view3, m_view4, m_view5;
    tavros::ui::view      m_view6, m_view7, m_view8, m_view9, m_view10, m_view11;
    tavros::ui::button    m_btn0, m_btn1;

    // -- Particles --
    app::particle_archetype m_particles;
    tavros::math::vec3      m_meteor_pos = {-10.0f, 8.0f, 0.0f};
    tavros::math::vec3      m_meteor_vel = {4.0f, -2.0f, 1.0f};

    // -- Input --
    tavros::input::input_manager m_input_manager;

    // -- Frame state --
    tavros::core::timer m_timer;

    size_t    m_frame_number = 0;
    fps_meter m_fps_meter;
    float     m_delta_time = 0.0f;
    float     m_time = 0.0f;
    uint32    m_current_buffer_output = 0;
    bool      m_capture_next_frame = false;
};

// -------------------------------------------------------------------------
// Entry point
// -------------------------------------------------------------------------

int main()
{
    tavros::core::logger::add_consumer(
        [](auto, auto, auto msg) { std::printf("%s\n", msg.data()); }
    );

    auto rm = tavros::core::make_shared<resource_manager>();
    auto wnd = tavros::core::make_unique<main_window>("TavrosEngine", rm);
    wnd->run_render_loop();

    return tavros::system::application::instance().run();
}

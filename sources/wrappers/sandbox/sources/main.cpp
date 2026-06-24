#include "render_app_base.hpp"

#include "scene_data.hpp"
#include "free_camera.hpp"

#include <tavros/renderer/shaders/shader_loader.hpp>
#include <tavros/renderer/shaders/shader_source_provider.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/exception.hpp>
#include <tavros/core/filesystem.hpp>
#include <tavros/core/timer.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/fixed_string.hpp>
#include <tavros/renderer/render_target/render_target.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/resource_manager.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>
#include <tavros/renderer/gpu_buffer_view.hpp>
#include <tavros/renderer/gpu_stage_buffer.hpp>
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
#include <tavros/core/resource/object_pool.hpp>
#include <tavros/assets/providers/filesystem_provider.hpp>
#include <tavros/renderer/rhi/string_utils.hpp>
#include <tavros/tef/saver.hpp>
#include <tavros/tef/loader.hpp>
#include <tavros/tef/schema.hpp>

#include <tracy/Tracy.hpp>

#include <tavros/renderer/render_system.hpp>


constexpr uint32 k_msaa = 16;

namespace fs = tavros::filesystem;
namespace rhi = tavros::renderer::rhi;

static tavros::core::logger logger("main");

#define TAV_FATAL_IF(expr, msg)      \
    do {                             \
        if ((expr)) {                \
            logger.fatal("{}", msg); \
            ::std::abort();          \
        }                            \
    } while (0)

constexpr size_t operator""_MiB(unsigned long long v) noexcept
{
    return v * 1024ull * 1024ull;
}
constexpr size_t operator""_KiB(unsigned long long v) noexcept
{
    return v * 1024ull;
}

// -------------------------------------------------------------------------
// Asset providers
// -------------------------------------------------------------------------

class filesystem_shader_provider : public tavros::renderer::shader_source_provider
{
public:
    explicit filesystem_shader_provider(tavros::core::shared_ptr<tavros::assets::asset_manager> am)
        : m_am(std::move(am))
    {
    }

    tavros::core::string load(tavros::core::string_view path) override
    {
        return m_am->read_text(path);
    }

private:
    tavros::core::shared_ptr<tavros::assets::asset_manager> m_am;
};

class filesystem_font_provider : public tavros::renderer::font_data_provider
{
public:
    explicit filesystem_font_provider(tavros::core::shared_ptr<tavros::assets::asset_manager> am)
        : m_am(std::move(am))
    {
    }

    tavros::core::dynamic_buffer<uint8> load(tavros::core::string_view path) override
    {
        return m_am->read_binary(path);
    }

private:
    tavros::core::shared_ptr<tavros::assets::asset_manager> m_am;
};


class tavros_engine_file_provider : public tavros::tef::source_provider
{
public:
    explicit tavros_engine_file_provider(tavros::core::shared_ptr<tavros::assets::asset_manager> am)
        : m_am(std::move(am))
    {
    }

    tavros::core::string load(tavros::core::string_view path) override
    {
        return m_am->read_text(path);
    }

private:
    tavros::core::shared_ptr<tavros::assets::asset_manager> m_am;
};


// -------------------------------------------------------------------------
// Common depth-stencil presets
// -------------------------------------------------------------------------

inline rhi::depth_stencil_state depth_test_rw() noexcept
{
    rhi::depth_stencil_state s;
    s.format = rhi::pixel_format::depth32f;
    s.depth_test_enable = true;
    s.depth_write_enable = true;
    s.depth_compare = rhi::compare_op::less;
    return s;
}

inline rhi::depth_stencil_state depth_test_ro() noexcept
{
    rhi::depth_stencil_state s;
    s.format = rhi::pixel_format::depth32f;
    s.depth_test_enable = true;
    s.depth_write_enable = false;
    s.depth_compare = rhi::compare_op::less;
    return s;
}

inline rhi::depth_stencil_state depth_off() noexcept
{
    rhi::depth_stencil_state s;
    s.format = rhi::pixel_format::none;
    s.depth_test_enable = false;
    s.depth_write_enable = false;
    return s;
}

inline rhi::depth_stencil_state depth_on_write_off() noexcept
{
    rhi::depth_stencil_state s;
    s.format = rhi::pixel_format::depth32f;
    s.depth_test_enable = true;
    s.depth_write_enable = false;
    s.depth_compare = rhi::compare_op::less;
    return s;
}

inline rhi::depth_stencil_state depth_test_leq() noexcept
{
    rhi::depth_stencil_state s;
    s.format = rhi::pixel_format::depth32f;
    s.depth_compare = rhi::compare_op::less_equal;
    s.depth_test_enable = true;
    s.depth_write_enable = false;
    return s;
}

// -------------------------------------------------------------------------
// Common blend presets
// -------------------------------------------------------------------------

/// Standard alpha blending.
inline rhi::blend_state alpha_blend() noexcept
{
    return {true, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add};
}

/// Additive blending (fire/glow effects).
inline rhi::blend_state additive_blend() noexcept
{
    return {true, rhi::blend_factor::src_alpha, rhi::blend_factor::one, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one, rhi::blend_op::add};
}

/// No blending.
inline rhi::blend_state no_blend() noexcept
{
    return {false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add};
}

// -------------------------------------------------------------------------
// Common rasterizer presets
// -------------------------------------------------------------------------

inline rhi::rasterizer_state cull_back_ccw() noexcept
{
    rhi::rasterizer_state s;
    s.cull = rhi::cull_face::back;
    s.face = rhi::front_face::counter_clockwise;
    s.polygon = rhi::polygon_mode::fill;
    return s;
}

inline rhi::rasterizer_state cull_off() noexcept
{
    rhi::rasterizer_state s;
    s.cull = rhi::cull_face::off;
    s.polygon = rhi::polygon_mode::fill;
    return s;
}

inline rhi::rasterizer_state cull_off_scissor() noexcept
{
    auto s = cull_off();
    s.scissor_enable = true;
    return s;
}

// -------------------------------------------------------------------------
// Default multisample (off)
// -------------------------------------------------------------------------

inline rhi::multisample_state no_msaa() noexcept
{
    rhi::multisample_state s;
    s.sample_shading_enabled = false;
    s.sample_count = 1;
    s.min_sample_shading = 0.0f;
    return s;
}


// -------------------------------------------------------------------------
// main_window
// -------------------------------------------------------------------------

class main_window : public app::render_app_base
{
public:
    main_window(tavros::core::string_view name, tavros::core::shared_ptr<tavros::assets::asset_manager> am)
        : app::render_app_base(name)
        , m_am(am)
        , m_sl(tavros::core::make_unique<filesystem_shader_provider>(am))
    {
        auto loader = tavros::tef::loader(std::move(tavros::core::make_unique<tavros_engine_file_provider>(am)));
        auto config = loader.load("config.tef");
        m_ws = std::move(config);
        TAV_FATAL_IF(!m_ws, "falied to load config.tef");
    }

    ~main_window() override = default;

    // ------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------

    void init() override
    {
        ZoneScopedNC("AppInit", 0x607D8B);
        init_graphics();
        show();
    }

    void shutdown() override
    {
        ZoneScopedNC("AppShutdown", 0x607D8B);
        m_uniform_buffer.shutdown();
        m_renderer->shutdown();
    }

    // ------------------------------------------------------------------
    // Render loop
    // ------------------------------------------------------------------

    void render(tavros::input::event_args_queue_view events, std::chrono::microseconds time_us) override
    {
        ZoneScopedNC("Frame", 0xFFFFFF);

        process_input(events, time_us);

        m_renderer->begin_frame();
        m_composer->begin_frame();

        auto* cbuf = m_composer->create_command_queue();
        update_frame_data();

        m_uniform_buffer.reset();
        auto scene_data_slice = m_uniform_buffer.slice<app::scene_data>(1);
        scene_data_slice.data().copy_from(&m_scene_data, 1);

        cbuf->begin_rendering(m_offscreen_rt->framebuffer());
        cbuf->bind_shader_buffers(rhi::buffer_binding{scene_data_slice.gpu_buffer(), static_cast<uint32>(scene_data_slice.offset_bytes()), static_cast<uint32>(scene_data_slice.size_bytes()), 0});

        cbuf->bind_pipeline(m_skybox_pipeline);
        auto tex = m_sky_textures[m_sky_index];
        cbuf->bind_shader_textures(rhi::texture_binding{tex->gpu_texture, m_sampler, 0});
        cbuf->draw(36);

        draw_world_grid(*cbuf, 0);

        cbuf->end_rendering();


        cbuf->begin_rendering(m_composer->backbuffer());
        cbuf->bind_pipeline(m_fullscreen_quad_pipeline);
        cbuf->bind_shader_textures(rhi::texture_binding{m_offscreen_rt->color_attachments()[0], m_sampler, 0});
        cbuf->draw(4);
        cbuf->end_rendering();

        m_renderer->end_frame();
        m_composer->end_frame();
        m_composer->wait_for_frame_complete();
        m_composer->present();

        m_input_manager.end_frame();

        ++m_frame_number;

        FrameMark;
    }

private:
    // ==================================================================
    // Init helpers
    // ==================================================================

    void init_graphics()
    {
        ZoneScopedNC("InitGraphics", 0x607D8B);

        m_renderer = tavros::core::make_unique<tavros::renderer::render_system>(m_am, m_ws);
        TAV_FATAL_IF(!m_renderer, "Failed to create render system.");

        m_renderer->init(native_handle());


        m_composer = m_renderer->get_frame_composer();
        TAV_FATAL_IF(!m_composer, "Failed to get frame composer pointer.");

        m_free_cam.set_orientation(tavros::math::quat::look_rotation(tavros::math::vec3(-1.0f, -1.0f, -0.5f), tavros::math::vec3(0.0f, 0.0f, 1.0f)));
        m_free_cam.set_orbit_dist(30.0f);

        m_offscreen_rt = m_renderer->resource_manager()->load<tavros::renderer::render_target>("main_offscreen_rendertarget");
        TAV_FATAL_IF(!m_offscreen_rt, "Failed to create offscreen render target.");


        rhi::sampler_create_info si;
        si.filter.mipmap_filter = rhi::mipmap_filter_mode::linear;
        si.filter.min_filter = rhi::filter_mode::linear;
        si.filter.mag_filter = rhi::filter_mode::linear;
        si.wrap_mode.wrap_s = rhi::wrap_mode::clamp_to_edge;
        si.wrap_mode.wrap_t = rhi::wrap_mode::clamp_to_edge;
        si.wrap_mode.wrap_r = rhi::wrap_mode::clamp_to_edge;
        m_sampler = m_renderer->get_graphics_device()->create_sampler(si);
        TAV_FATAL_IF(!m_sampler, "Failed to create sampler.");


        m_fullscreen_quad_pipeline = build_fullscreen_quad_pipeline();
        m_world_grid_pipeline = build_world_grid_pipeline();
        m_skybox_pipeline = build_skybox_pipeline();

        m_uniform_buffer.init(m_renderer->get_graphics_device(), 256_MiB, rhi::buffer_usage::constant);

        m_sky_textures.push_back(m_renderer->resource_manager()->load<tavros::renderer::texture_t>("textures.cloudy_sky"));
        m_sky_textures.push_back(m_renderer->resource_manager()->load<tavros::renderer::texture_t>("textures.cloudy_sunset_sky"));
        m_sky_textures.push_back(m_renderer->resource_manager()->load<tavros::renderer::texture_t>("textures.dark_sky"));
        m_sky_textures.push_back(m_renderer->resource_manager()->load<tavros::renderer::texture_t>("textures.pure_sunset_sky"));
        m_sky_textures.push_back(m_renderer->resource_manager()->load<tavros::renderer::texture_t>("textures.pure_sky"));
        m_sky_textures.push_back(m_renderer->resource_manager()->load<tavros::renderer::texture_t>("textures.sunset_sky"));
    }

    // ==================================================================
    // Per-frame helpers
    // ==================================================================

    void process_input(tavros::input::event_args_queue_view events, std::chrono::microseconds time_us)
    {
        ZoneScopedNC("InputProcessEvents", 0x9C27B0);
        m_input_manager.begin_frame(tavros::system::application::instance().highp_time_us());
        m_input_manager.process_events(events);
        handle_window_resize();
        m_free_cam.update(m_input_manager, time_us);

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_equal)) {
            m_sky_index += 1;
            if (m_sky_index >= m_sky_textures.size()) {
                m_sky_index = 0;
            }
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_minus)) {
            m_sky_index -= 1;
            if (m_sky_index < 0) {
                m_sky_index = m_sky_textures.size() - 1;
            }
        }
    }

    void handle_window_resize()
    {
        if (!m_input_manager.is_window_resized()) {
            return;
        }
        const auto sz = m_input_manager.window_size();
        if (sz.width == 0 || sz.height == 0) {
            return;
        }
        TracyMessageL("WindowResize");

        m_offscreen_rt->resize(static_cast<uint32>(sz.width), static_cast<uint32>(sz.height), k_msaa);

        m_composer->resize(sz.width, sz.height);
        const float fov_y = 70.0f * 3.14159265358979f / 180.0f;
        const float near = 0.1f;
        const float far = 5000.0f;
        m_free_cam.set_perspective(fov_y, static_cast<float>(sz.width) / static_cast<float>(sz.height), near, far);
    }

    void update_frame_data()
    {
        const float w = static_cast<float>(m_input_manager.window_size().width);
        const float h = static_cast<float>(m_input_manager.window_size().height);

        const auto& cam = m_free_cam.camera();
        m_scene_data.view = cam.view_matrix();
        m_scene_data.projection = cam.projection_matrix();
        m_scene_data.view_projection = cam.view_projection_matrix();
        m_scene_data.inverse_view = tavros::math::inverse(cam.view_matrix());
        m_scene_data.inverse_projection = tavros::math::inverse(cam.projection_matrix());
        m_scene_data.ortho_projection = tavros::math::mat4::ortho(0.0f, w, h, 0.0f, 1.0f, -1.0f);
        m_scene_data.camera_position = cam.position();
        m_scene_data.near_plane = cam.perspective_params().z_near;
        m_scene_data.far_plane = cam.perspective_params().z_far;
        m_scene_data.aspect_ratio = cam.perspective_params().aspect;
        m_scene_data.fov_y = cam.perspective_params().fov_y;
        m_scene_data._pad0 = 0.0f;
        m_scene_data.frame_size = {w, h};
        m_scene_data.time = 0.0f;
        m_scene_data.delta_time = 0.0f;
        m_scene_data.frame_index = static_cast<uint32>(m_frame_number);
        m_scene_data._pad1 = 0.0f;
    }

    rhi::pipeline_handle build_fullscreen_quad_pipeline()
    {
        rhi::pipeline_create_info info;
        info.shader_program = load_and_create_shader_program("tavros/shaders/fullscreen_quad.vert", "tavros/shaders/fullscreen_quad.frag");
        info.depth_stencil_attachment = depth_off();
        info.rasterizer = cull_off();
        info.topology = rhi::primitive_topology::triangle_strip;
        info.color_attachments.push_back({rhi::pixel_format::rgba8un, rhi::k_rgba_color_mask, alpha_blend()});

        auto h = m_renderer->get_graphics_device()->create_pipeline(info);
        m_renderer->get_graphics_device()->destroy_shader(info.shader_program);
        TAV_FATAL_IF(!h, "Failed to create fullscreen quad pipeline.");
        return h;
    }

    void draw_world_grid(rhi::command_queue& cbuf, uint32 plane = 0)
    {
        using namespace tavros;

        struct shader_data
        {
            uint32 minor_color = 0;
            uint32 major_color = 0;
            uint32 axis_u_color = 0;
            uint32 axis_v_color = 0;
            float  minor_line_width = 0.0f;
            float  major_line_width = 0.0f;
            float  axis_line_width = 0.0f;
            float  minor_grid_step = 0.0f;
            float  major_grid_step = 0.0f;
            uint32 flags = 0; // Флаги. flags & 0x3 == (0 - плоскость XY; 1 - плоскость YZ; 2 - плоскость ZX)
        };

        uint32 u_color = 0;
        uint32 v_color = 0;

        constexpr auto x_axis_color = math::vec4(0.6549f, 0.2196f, 0.3255f, 1.0f);
        constexpr auto y_axis_color = math::vec4(0.4196f, 0.5569f, 0.1373f, 1.0f);
        constexpr auto z_axis_color = math::vec4(0.2314f, 0.5137f, 0.7412f, 1.0f);

        auto plane_id = plane & 0x3;
        if (plane_id == 0) { // XY plane
            u_color = math::rgba8(x_axis_color).color;
            v_color = math::rgba8(y_axis_color).color;
        } else if (plane_id == 1) { // YZ plane
            u_color = math::rgba8(y_axis_color).color;
            v_color = math::rgba8(z_axis_color).color;
        } else if (plane_id == 2) { // ZX plane
            u_color = math::rgba8(z_axis_color).color;
            v_color = math::rgba8(x_axis_color).color;
        }
        // TODO
        shader_data sd;
        sd.minor_color = math::rgba8(75, 75, 75, 76).color;
        sd.major_color = math::rgba8(100, 100, 100, 76).color;
        sd.axis_u_color = u_color;
        sd.axis_v_color = v_color;
        sd.minor_line_width = 1.0f;
        sd.major_line_width = 3.0f;
        sd.axis_line_width = 5.0f;
        sd.minor_grid_step = 10.0f;
        sd.major_grid_step = 100.0f;
        sd.flags = plane_id;

        cbuf.bind_pipeline(m_world_grid_pipeline);
        cbuf.push_constant(sd);
        cbuf.draw(4);
    }

    rhi::pipeline_handle build_skybox_pipeline()
    {
        rhi::pipeline_create_info info;
        info.shader_program = load_and_create_shader_program("tavros/shaders/skybox.vert", "tavros/shaders/skybox.frag");
        info.depth_stencil_attachment = depth_test_leq();
        info.rasterizer = cull_off();
        info.topology = rhi::primitive_topology::triangles;
        info.color_attachments.push_back({rhi::pixel_format::rgba8un, rhi::k_rgba_color_mask, no_blend()});
        info.multisample = no_msaa();

        auto h = m_renderer->get_graphics_device()->create_pipeline(info);
        m_renderer->get_graphics_device()->destroy_shader(info.shader_program);
        TAV_FATAL_IF(!h, "Failed to create Skybox pipeline.");
        return h;
    }

    rhi::pipeline_handle build_world_grid_pipeline()
    {
        rhi::pipeline_create_info info;
        info.shader_program = load_and_create_shader_program("tavros/shaders/world_grid.vert", "tavros/shaders/world_grid.frag");
        info.depth_stencil_attachment = depth_test_ro();
        info.rasterizer = cull_off();
        info.topology = rhi::primitive_topology::triangle_strip;
        info.color_attachments.push_back({rhi::pixel_format::rgba8un, rhi::k_rgba_color_mask, alpha_blend()});
        info.multisample = no_msaa();

        auto h = m_renderer->get_graphics_device()->create_pipeline(info);
        m_renderer->get_graphics_device()->destroy_shader(info.shader_program);
        TAV_FATAL_IF(!h, "Failed to create world grid pipeline.");
        return h;
    }

    rhi::shader_handle load_and_create_shader_program(tavros::core::string_view vs_path, tavros::core::string_view fs_path)
    {
        const auto src_vs = m_sl.load(vs_path, {tavros::renderer::shader_language::glsl_460, ""});
        const auto src_fs = m_sl.load(fs_path, {tavros::renderer::shader_language::glsl_460, ""});
        auto       sh = m_renderer->get_graphics_device()->create_shader({src_vs, src_fs});
        return m_renderer->get_graphics_device()->create_shader({src_vs, src_fs});
    }

    // ==================================================================
    // Members
    // ==================================================================

    tavros::core::shared_ptr<tavros::tef::workspace>          m_ws;
    tavros::core::shared_ptr<tavros::assets::asset_manager>   m_am;
    tavros::core::unique_ptr<tavros::renderer::render_system> m_renderer;

    // -- Core --
    tavros::renderer::shader_loader m_sl;

    // -- Graphics device --
    rhi::frame_composer* m_composer = nullptr;

    // -- Scene --
    tavros::sandbox::free_camera m_free_cam;
    app::scene_data              m_scene_data;

    tavros::renderer::gpu_stream_buffer m_uniform_buffer;

    tavros::renderer::render_target_ref                 m_offscreen_rt;
    tavros::core::vector<tavros::renderer::texture_ref> m_sky_textures;
    int32                                               m_sky_index = 0;

    tavros::renderer::rhi::sampler_handle m_sampler;

    tavros::renderer::rhi::pipeline_handle m_fullscreen_quad_pipeline;
    tavros::renderer::rhi::pipeline_handle m_world_grid_pipeline;
    tavros::renderer::rhi::pipeline_handle m_skybox_pipeline;


    // -- Input --
    tavros::input::input_manager m_input_manager;

    //
    uint64 m_frame_number = 0;
};

// -------------------------------------------------------------------------
// Entry point
// -------------------------------------------------------------------------

int main()
{
    tavros::core::logger::add_consumer(
        [](auto, auto, auto msg) { std::printf("%s\n", msg.data()); }
    );

    auto am = tavros::core::make_shared<tavros::assets::asset_manager>();
    am->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH, "");
    am->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH, "file");
    am->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH "/shaders", "");
    am->mount<tavros::assets::filesystem_provider>(TAV_ASSETS_PATH "/shaders", "file");
    am->mount<tavros::assets::filesystem_provider>(TAV_OUTPUT_PATH, "output", false, true);

    auto wnd = tavros::core::make_unique<main_window>("TavrosEngine", am);
    wnd->run_render_loop();

    return tavros::system::application::instance().run();
}
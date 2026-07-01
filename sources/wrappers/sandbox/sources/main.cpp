#include "render_app_base.hpp"

#include "free_camera.hpp"

#include <tavros/assets/asset_manager.hpp>
#include <tavros/assets/providers/filesystem_provider.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/math.hpp>
#include <tavros/renderer/render_system.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>
#include <tavros/system/application.hpp>
#include <tavros/tef/loader.hpp>
#include <tavros/input/input_manager.hpp>

#include <tavros/renderer/text/text_builder.hpp>
#include <tavros/renderer/text/text_layouter.hpp>

#include <tracy/Tracy.hpp>


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

// -------------------------------------------------------------------------
// Asset providers
// -------------------------------------------------------------------------

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


struct alignas(16) scene_data
{
    // Matrices
    tavros::math::mat4 view;
    tavros::math::mat4 projection;
    tavros::math::mat4 view_projection;
    tavros::math::mat4 inverse_view;
    tavros::math::mat4 inverse_projection;
    tavros::math::mat4 ortho_projection;

    // Camera
    tavros::math::vec3 camera_position;
    float              _pad0 = 0.0f;
    float              near_plane;

    float far_plane;
    float aspect_ratio;
    float fov_y;
    float _pad1 = 0.0f;

    // Frame
    tavros::math::vec2 frame_size;
    tavros::math::vec2 frame_size_inv;

    float    time = 0.0f;
    float    delta_time = 0.0f;
    uint32_t frame_index = 0;
    float    _pad2 = 0.0f;
};

enum class brush_type : int32
{
    solid = 0,
    linear_gradient = 1,
    radial_gradient = 2,
};

static_assert(sizeof(scene_data) % 16 == 0, "scene_data must be 16-byte aligned");

using tavros::math::vec2;
using tavros::math::vec3;
using tavros::math::vec4;

// -------------------------------------------------------------------------
// main_window
// -------------------------------------------------------------------------

class main_window : public app::render_app_base
{
public:
    main_window(tavros::core::string_view name, tavros::core::shared_ptr<tavros::assets::asset_manager> am)
        : app::render_app_base(name)
        , m_am(am)
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

    tavros::core::string_view smp_to_str(tavros::renderer::sampler_preset smp) noexcept
    {
        using sp = tavros::renderer::sampler_preset;
        switch (smp) {
        case sp::automatic:
            return "automatic";
        case sp::nearest_clamp:
            return "nearest_clamp";
        case sp::nearest_repeat:
            return "nearest_repeat";
        case sp::linear_clamp:
            return "linear_clamp";
        case sp::linear_repeat:
            return "linear_repeat";
        case sp::trilinear_clamp:
            return "trilinear_clamp";
        case sp::trilinear_repeat:
            return "trilinear_repeat";
        case sp::shadow:
            return "shadow";
        case sp::shadow_pcf:
            return "shadow_pcf";
        default:
            return "<unknown>";
        }
    }

    // ------------------------------------------------------------------
    // Render loop
    // ------------------------------------------------------------------

    float tm = 0.0f;

    vec4 hex_color(uint32 hex) noexcept
    {
        auto c = tavros::math::rgba8(hex);
        return {static_cast<float>(c.r) / 255.0f, static_cast<float>(c.g) / 255.0f, static_cast<float>(c.b) / 255.0f, static_cast<float>(c.a) / 255.0f};
    }

    void render(tavros::input::event_args_queue_view events, std::chrono::microseconds time_us) override
    {
        ZoneScopedNC("Frame", 0xFFFFFF);

        tm += time_us.count() / 1'000'000.0f;

        process_input(events, time_us);

        m_renderer->begin_frame();

        auto* cbuf = m_renderer->get_graphics_device()->create_command_queue();
        update_frame_data();

        m_uniform_buffer.reset();
        auto scene_data_slice = m_uniform_buffer.slice<scene_data>(1);
        scene_data_slice.data().copy_from(&m_scene_data, 1);

        cbuf->begin_rendering(m_offscreen_rt->gpu_framebuffer());
        cbuf->bind_shader_buffers(rhi::buffer_binding{scene_data_slice.gpu_buffer(), static_cast<uint32>(scene_data_slice.offset_bytes()), static_cast<uint32>(scene_data_slice.size_bytes()), 0});

        auto smp = m_renderer->resource_manager()->sampler(static_cast<tavros::renderer::sampler_preset>(m_smp_index));
        cbuf->bind_pipeline(m_skybox_pipeline->gpu_pipeline());
        auto tex = m_sky_textures[m_sky_index];
        cbuf->bind_shader_textures(rhi::texture_binding{tex->gpu_texture(), smp, 0});
        cbuf->draw(36);

        draw_world_grid(*cbuf, 0);

        set_brush_gradient(*cbuf, {600.0f, 400.0f}, {m_input_manager.mouse_pos().x, m_input_manager.mouse_pos().y}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 0.1f});

        auto* r = m_renderer->renderer2d();

        r->set_line_thickness(3.0f);
        r->set_brush_solid_color(0x00C6EEFF);
        r->move_to({600.0f, 400.0f});
        r->draw_line_to({700.0f, 400.0f});

        r->set_brush_solid_color(0x00C606FF);
        r->fill_circle({100, 100}, 50);
        r->set_brush_solid_color(0xC40D8180);
        r->fill_ring({300, 60}, 50, 40);
        r->set_brush_solid_color(0xBF262695);
        r->fill_ring_cut({400, 60}, 50, 60, {20, 20});


        r->set_brush_radial_gradient({600, 50}, 0x3E4F3FFF, 25, 0x3E4F3F33);
        r->fill_circle({600, 50}, 15);
        r->set_brush_radial_gradient({600, 50}, 0xFFFFFF00, 25, 0xFFFFFF40);
        r->fill_ring_cut({600, 50}, 15, 20, {0, 10});
        r->set_brush_linear_gradient({600, 25}, 0x97D46BFF, {600, 75}, 0x5B9238FF);
        r->fill_circle({600, 50}, 12);

        r->fill_rect({800, 100}, {50, 20}, 20);

        r->set_sprite_pivot({0.5f, 0.5f});
        r->set_brush_solid_color(0xFFFFFFFF);
        r->draw_sprite(m_ui_texture, {400.0f, 400.0f}, {123.0f * 2, 87.0f * 2}, 0.0f, {492.0f, 435.0f, 123.0f, 87.0f});
        r->draw_sprite(m_ui_texture, {700.0f, 400.0f}, {123.0f * 2, 87.0f * 2}, 0.0f, {369.0f, 522.0f, 123.0f, 87.0f});
        r->draw_sprite(m_ui_texture, {1000.0f, 400.0f}, {123.0f * 2, 87.0f * 2}, 0.0f, {615.0f, 175.0f, 123.0f, 87.0f});


        m_text.clear();
        m_text.set_text_align(tavros::renderer::text_align::left);
        auto secs = static_cast<uint32>(tm);
        auto s = secs / 5;
        if (s % 2 == 0) {
            m_text.append_text("Постоянно меняющийся текст!\nПривет Мир!");
        } else {
            m_text.append_text("Другой меняющийся текст!\nИ пару новых строк!\nЕще одна строка!");
        }


        const auto p = tavros::math::vec2(500, 500);

        m_text.set_text_align(tavros::renderer::text_align::center);
        auto l = m_text.layout();
        r->set_brush_solid_color(0x33333355);
        //  r->draw_aabb({l.min + p, l.max + p});

        r->fill_rect(l.center() + p, l.size() / 2.0f, 3.0f);

        r->set_brush_solid_color(0x33333388);
        r->fill_rect_cut(l.center() + p, l.size() / 2.0f + 2.0f, {0.0f, 0.0f}, l.size() / 2.0f, 5.0f, 3.0f);

        r->set_text_use_brush_mask(true, true);
        r->set_brush_radial_gradient(m_input_manager.mouse_pos(), 0xFF0000FF, 100, 0xFFFFFFFF);
        r->draw_text(m_text.text(), {500, 500}, 0.5, 0.1);

        r->set_line_thickness(3);


        r->set_brush_solid_color(0xFFFFFFFF);
        r->draw_line(p, p + tavros::math::vec2{-10, 0});
        r->draw_line(p, p + tavros::math::vec2{10, 0});
        r->draw_line(p, p + tavros::math::vec2{0, -10});
        r->draw_line(p, p + tavros::math::vec2{0, 10});

        draw_world_axis({m_input_manager.window_size().width - 120.0f, 120.0f}, 90.0f);


        cbuf->end_rendering();

        cbuf->begin_rendering(m_renderer->gpu_backbuffer());
        cbuf->bind_pipeline(m_fullscreen_quad_pipeline->gpu_pipeline());
        cbuf->bind_shader_textures(rhi::texture_binding{m_offscreen_rt->color_attachments()[0], smp, 0});
        cbuf->draw(4);
        cbuf->end_rendering();

        m_renderer->get_graphics_device()->submit_command_queue(cbuf);
        m_renderer->end_frame();
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

        m_free_cam.set_orientation(tavros::math::quat::look_rotation(tavros::math::vec3(-1.0f, -1.0f, -0.5f), tavros::math::vec3(0.0f, 0.0f, 1.0f)));
        m_free_cam.set_orbit_dist(30.0f);

        auto* rm = m_renderer->resource_manager();

        m_offscreen_rt = rm->load_render_target("rt.main_offscreen");
        TAV_FATAL_IF(!m_offscreen_rt, "Failed to create offscreen render target.");

        m_uniform_buffer.init(m_renderer->get_graphics_device(), 256 * 1024 * 1024, rhi::buffer_usage::constant);

        m_sky_textures.push_back(rm->load_texture("tex.cloudy_sky"));
        m_sky_textures.push_back(rm->load_texture("tex.misty_morning"));
        // m_sky_textures.push_back(rm->load_texture("tex.cloudy_sunset_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.dark_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.pure_sunset_sky"));
        m_sky_textures.push_back(rm->load_texture("tex.pure_sky"));
        m_sky_textures.push_back(rm->load_texture("tex.sunset_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.rock_sky"));
        // m_sky_textures.push_back(rm->load_texture("tex.rock_sky_small"));

        m_ui_texture = rm->load_texture("tex.ui_actor_portrets");

        rm->set_material_load_params({}, 1, rhi::pixel_format::depth32f);
        m_skybox_pipeline = rm->load_material("mt.skybox");
        m_world_grid_pipeline = rm->load_material("mt.world_grid");
        rm->set_material_load_params({}, 1, rhi::pixel_format::none);
        m_fullscreen_quad_pipeline = rm->load_material("mt.fullscreen_quad");

        m_font = rm->load_font("fnt.consola_mono");
        // m_font = rm->load_font("fnt.droid_sans");
        m_font = rm->load_font("fnt.home_video");
        // m_font = rm->load_font("fnt.noto_sans_regular");
        m_font = rm->load_font("fnt.roboto_medium");

        m_text.set_font(m_font);
        m_text.set_font_size(64.0f);
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

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_pagedown)) {
            m_smp_index += 1;
            if (m_smp_index >= static_cast<int32>(tavros::renderer::sampler_preset::count)) {
                m_smp_index = 0;
            }
            logger.info("Sampler selected: {}", smp_to_str(static_cast<tavros::renderer::sampler_preset>(m_smp_index)));
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

        m_renderer->resize_backbuffer(sz.width, sz.height);
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

    void set_brush_gradient(rhi::command_queue& cmd, vec2 p0, vec2 p1, vec4 color0, vec4 color1)
    {
        set_brush(cmd, brush_type::linear_gradient, p0, p1, color0, color1);
    }

    void set_brush_radial_gradient(rhi::command_queue& cmd, vec2 center, vec2 target, vec4 color_center, vec4 color_target)
    {
        set_brush(cmd, brush_type::radial_gradient, center, target, color_center, color_target);
    }

    void set_brush_solid(rhi::command_queue& cmd, vec4 color)
    {
        set_brush(cmd, brush_type::solid, {0.0f, 0.0f}, {0.0f, 0.0f}, color, color);
    }

    void set_brush(rhi::command_queue& cmd, brush_type type, vec2 p0, vec2 p1, vec4 color0, vec4 color1)
    {
        struct brush_data
        {
            vec2  pos0;
            vec2  pos1;
            vec4  color0;
            vec4  color1;
            int32 type;
        };

        const auto bd = brush_data{p0, p1, color0, color1, static_cast<int32>(type)};
        auto       slice = m_uniform_buffer.slice<brush_data>(1);
        slice.data().copy_from(&bd, 1);
        cmd.bind_shader_buffers(rhi::buffer_binding{slice.gpu_buffer(), static_cast<uint32>(slice.offset_bytes()), static_cast<uint32>(slice.size_bytes()), 1});
    }

    void bind_texture(rhi::command_queue& cmd, tavros::renderer::texture_ref tex, uint32 binding_point, tavros::renderer::sampler_preset smp)
    {
        auto sampler = m_renderer->resource_manager()->sampler(smp);
        cmd.bind_shader_textures(rhi::texture_binding{tex->gpu_texture(), sampler, binding_point});
    }

    void draw_world_axis(const tavros::math::vec2& pos, float radius)
    {
        using namespace tavros;
        float line_width = 6.0f;
        float dash_size = 10.0f;
        float gap_size = 5.0f;
        float p_side_circle = 19.0f;
        float n_side_circle = 13.0f;
        float axis_len = radius - p_side_circle - line_width;

        auto view3 = math::mat3(m_free_cam.camera().view_matrix());

        auto project_axis = [&](math::vec3 world_dir) -> math::vec2 {
            math::vec3 cam = view3 * world_dir;
            return math::vec2(cam.x, -cam.y) * axis_len;
        };

        auto depth_of = [&](math::vec3 world_dir) -> float {
            return (view3 * world_dir).z;
        };

        constexpr auto x_color = math::rgba8(0xA73A53FF);
        constexpr auto y_color = math::rgba8(0x6BB023FF);
        constexpr auto z_color = math::rgba8(0x3A83BEFF);

        struct axis_info
        {
            math::vec2  dir;   // projected 2d direction
            math::rgba8 color;
            float       depth; // camera-space Z (больше = дальше)
            char        pos_label[3];
            char        neg_label[3];
            float       pos_font_size;
            float       neg_font_size;
        };

        axis_info axes[3] = {
            {project_axis({1, 0, 0}), x_color, depth_of({1, 0, 0}), "X", "-X", 36.0f, 22.0f},
            {project_axis({0, 1, 0}), y_color, depth_of({0, 1, 0}), "Y", "-Y", 36.0f, 22.0f},
            {project_axis({0, 0, 1}), z_color, depth_of({0, 0, 1}), "Z", "-Z", 36.0f, 22.0f},
        };

        // Сортируем: сначала рисуем оси с большей глубиной (дальше от камеры)
        int order[3] = {0, 1, 2};
        std::sort(std::begin(order), std::end(order), [&](int a, int b) {
            return axes[a].depth > axes[b].depth;
        });

        auto* r = m_renderer->renderer2d();

        // Фон
        r->set_brush_solid_color(0xF0F0F040);
        r->fill_circle(pos, radius);
        r->set_brush_solid_color(0xB0B0B070);
        r->fill_ring(pos, radius, radius - line_width);

        r->set_line_thickness(line_width);

        // Рисуем в порядке от дальнего к ближнему
        // Для каждой оси: сначала негативный конец (он дальше если depth > 0), потом позитивный
        tavros::renderer::rich_text text;
        text.set_font(m_font);
        r->set_text_use_brush_mask(false, false);

        auto draw_axis_neg = [&](const axis_info& ax) {
            r->set_brush_solid_color(ax.color);
            r->set_line_dash(dash_size, gap_size);
            r->draw_line(pos, pos - ax.dir);
            r->set_line_dash(0.0f, 0.0f);
            r->fill_circle(pos - ax.dir, n_side_circle);

            text.clear();
            text.set_font_size(ax.neg_font_size);
            text.append_text(ax.neg_label);
            auto l = text.layout();
            r->draw_text(text.text(), pos - ax.dir - l.center(), 0.5f, 0.0f);
        };

        auto draw_axis_pos = [&](const axis_info& ax) {
            r->set_brush_solid_color(ax.color);
            r->set_line_dash(0.0f, 0.0f);
            r->draw_line(pos, pos + ax.dir);
            r->fill_circle(pos + ax.dir, p_side_circle);

            text.clear();
            text.set_font_size(ax.pos_font_size);
            text.append_text(ax.pos_label);
            auto l = text.layout();
            r->draw_text(text.text(), pos + ax.dir - l.center(), 0.5f, 0.15f);
        };

        // Painter's algorithm:
        // Сначала все негативные концы дальних осей, потом позитивные ближних
        for (int i : order) {
            // depth > 0 означает что позитивный конец дальше — рисуем neg последним
            if (axes[i].depth > 0.0f) {
                draw_axis_neg(axes[i]);
            } else {
                draw_axis_pos(axes[i]);
            }
        }

        // Теперь в обратном порядке — ближние концы поверх
        for (int i = 2; i >= 0; --i) {
            if (axes[order[i]].depth > 0.0f) {
                draw_axis_pos(axes[order[i]]);
            } else {
                draw_axis_neg(axes[order[i]]);
            }
        }
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
            uint32 flags = 0; // flags & 0x3 == (0 - plane XY; 1 - plane YZ; 2 - plane ZX)
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

        cbuf.bind_pipeline(m_world_grid_pipeline->gpu_pipeline());
        cbuf.push_constant(sd);
        cbuf.draw(4);
    }


    // ==================================================================
    // Members
    // ==================================================================

    tavros::core::shared_ptr<tavros::tef::workspace>          m_ws;
    tavros::core::shared_ptr<tavros::assets::asset_manager>   m_am;
    tavros::core::unique_ptr<tavros::renderer::render_system> m_renderer;

    // -- Scene --
    tavros::sandbox::free_camera m_free_cam;
    scene_data                   m_scene_data;

    tavros::renderer::gpu_stream_buffer m_uniform_buffer;

    tavros::renderer::render_target_ref                 m_offscreen_rt;
    tavros::core::vector<tavros::renderer::texture_ref> m_sky_textures;
    int32                                               m_sky_index = 0;
    int32                                               m_smp_index = 0;

    tavros::renderer::texture_ref m_ui_texture;

    tavros::renderer::material_ref m_fullscreen_quad_pipeline;
    tavros::renderer::material_ref m_world_grid_pipeline;
    tavros::renderer::material_ref m_skybox_pipeline;

    tavros::renderer::font_ref m_font;

    tavros::renderer::rich_text m_text;

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
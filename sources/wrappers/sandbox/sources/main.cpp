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

#include <tavros/ui/root_view.hpp>
#include <tavros/ui/linear_view.hpp>
#include <tavros/ui/static/label.hpp>

#include <tracy/Tracy.hpp>

constexpr uint32 k_msaa = 1;

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

class main_window : public app::render_app_base
{
public:
    main_window(tavros::core::string_view name, tavros::core::shared_ptr<tavros::assets::asset_manager> am)
        : app::render_app_base(name)
        , m_am(am)
    {
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

        auto loader = tavros::tef::loader(std::move(tavros::core::make_unique<tavros_engine_file_provider>(am)));
        auto config = loader.load("config.tef");
        m_ws = std::move(config);
        TAV_FATAL_IF(!m_ws, "falied to load config.tef");
    }

    ~main_window() override = default;
    
    static constexpr tavros::math::vec4 to_vec4(tavros::math::rgba8 rgba) noexcept
    {
        return tavros::math::vec4(
            static_cast<float>(rgba.r) / 255.0f,
            static_cast<float>(rgba.g) / 255.0f,
            static_cast<float>(rgba.b) / 255.0f,
            static_cast<float>(rgba.a) / 255.0f
        );
    }

    static constexpr tavros::math::rgba8 to_rgba8(const tavros::math::vec4& color) noexcept
    {
        auto clamp01 = [](float v) noexcept -> float {
            return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        };

        auto to_u8 = [&](float v) noexcept -> uint8 {
            return static_cast<uint8>(clamp01(v) * 255.0f + 0.5f);
        };

        return tavros::math::rgba8{
            to_u8(color.x),
            to_u8(color.y),
            to_u8(color.z),
            to_u8(color.w),
        };
    }

    static tavros::math::rgba8 rainbow_color(float time_seconds) noexcept
    {
        static tavros::math::vec4 colors[] = {
            to_vec4(tavros::math::rgba8(0xFF5959FF)), // Red
            to_vec4(tavros::math::rgba8(0xFF9B59FF)), // Orange
            to_vec4(tavros::math::rgba8(0xFFFF59FF)), // Yellow
            to_vec4(tavros::math::rgba8(0x59FF59FF)), // Green
            to_vec4(tavros::math::rgba8(0x59FFFFFF)), // Blue
            to_vec4(tavros::math::rgba8(0x5959FFFF)), // Indigo
            to_vec4(tavros::math::rgba8(0xFF59E8FF))  // Violet
        };
        constexpr size_t count = sizeof(colors) / sizeof(colors[0]);

        // Заворачиваем время в диапазон [0, count)
        float t = std::fmod(time_seconds, static_cast<float>(count));
        if (t < 0.0f)
        {
            t += static_cast<float>(count);
        }

        size_t index_a = static_cast<size_t>(t);
        size_t index_b = (index_a + 1) % count;
        float  frac    = t - static_cast<float>(index_a);

        const tavros::math::vec4& a = colors[index_a];
        const tavros::math::vec4& b = colors[index_b];

        tavros::math::vec4 result{
            a.x + (b.x - a.x) * frac,
            a.y + (b.y - a.y) * frac,
            a.z + (b.z - a.z) * frac,
            a.w + (b.w - a.w) * frac,
        };

        return to_rgba8(result);
    }

    static constexpr tavros::math::rgba8 inv_color(tavros::math::rgba8 cl) noexcept
    {
        return tavros::math::rgba8(255 - cl.r, 255 - cl.g, 255 - cl.b, cl.a);
    }


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
        m_ui = nullptr;
        m_renderer->shutdown();
        m_renderer = nullptr;
    }

    // ------------------------------------------------------------------
    // Render loop
    // ------------------------------------------------------------------

    float time_sec = 0.0f;
    void render(tavros::input::event_args_queue_view events, std::chrono::microseconds time_us) override
    {
        ZoneScopedNC("Frame", 0xFFFFFF);

        process_input(events, time_us);

        m_ui->process_input(events);

        float delta_time_sec = static_cast<float>(time_us.count()) / 1'000'000.0f;
        time_sec += delta_time_sec;

        m_renderer->set_camera(m_free_cam.camera());
        auto frame = m_renderer->begin_frame(time_us.count());


        frame.commands().begin_rendering(m_offscreen_rt->gpu_framebuffer());

        auto smp = m_renderer->resource_manager()->sampler(static_cast<tavros::renderer::sampler_preset>(m_smp_index));
        frame.commands().bind_pipeline(m_skybox_pipeline->gpu_pipeline());
        auto tex = m_sky_textures[m_sky_index];
        frame.commands().bind_shader_textures(rhi::texture_binding{tex->gpu_texture(), smp, 0});
        frame.commands().draw(36);

        draw_world_grid(frame.commands(), 0);
        draw_world_axis({m_input_manager.window_size().width - 120.0f, 120.0f}, 90.0f);

        tavros::core::long_string lb_text;
        const auto                cam_pos = m_free_cam.camera().position();
        lb_text.fprint("Frame number: {}\nMouse pos: x={:.1f}   y={:.1f}\nCamera position: x={:.3f}   y={:.3f}   z={:.3f}", m_frame_number, m_input_manager.mouse_pos().x, m_input_manager.mouse_pos().y, cam_pos.x, cam_pos.y, cam_pos.z);

        m_label.set_text(lb_text);
        m_label.set_text_color(rainbow_color(time_sec));
        m_label.set_outline_color(inv_color(rainbow_color(time_sec)));
        

        if (static_cast<int32>(time_sec) % 10 < 5) {
            m_label.set_font_size(36.0f);
        } else {
            m_label.set_font_size(72.0f);
        }

        m_ui->resolve();
        m_ui->layout({0.0f, 0.0f, static_cast<float>(m_input_manager.window_size().width), static_cast<float>(m_input_manager.window_size().height)});
        m_ui->update(delta_time_sec);
        m_ui->draw();

        frame.commands().end_rendering();

        frame.commands().begin_rendering(m_renderer->gpu_backbuffer());
        frame.commands().bind_pipeline(m_fullscreen_quad_pipeline->gpu_pipeline());
        frame.commands().bind_shader_textures(rhi::texture_binding{m_offscreen_rt->color_attachments()[0], smp, 0});
        frame.commands().draw(4);
        frame.commands().end_rendering();

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

        m_renderer = tavros::core::make_shared<tavros::renderer::render_system>(m_am, m_ws);
        TAV_FATAL_IF(!m_renderer, "Failed to create render system.");

        m_renderer->init(native_handle());

        m_free_cam.set_orientation(tavros::math::quat::look_rotation(tavros::math::vec3(-1.0f, -1.0f, -0.5f), tavros::math::vec3(0.0f, 0.0f, 1.0f)));
        m_free_cam.set_orbit_dist(30.0f);

        auto* rm = m_renderer->resource_manager();

        m_offscreen_rt = rm->load_render_target("rt.main_offscreen");
        TAV_FATAL_IF(!m_offscreen_rt, "Failed to create offscreen render target.");


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
        //m_font = rm->load_font("fnt.home_video");
        // m_font = rm->load_font("fnt.noto_sans_regular");
        //m_font = rm->load_font("fnt.roboto_medium");

        m_ui = tavros::core::make_unique<tavros::ui::root_view>(m_renderer);
        m_ui->set_root(&m_root_view);
        m_root_view.set_padding({20.0f, 30.0f, 20.0f, 50.0f});
        m_root_view.set_content_orientation(tavros::ui::content_orient::vertical);
        m_horiz_view.set_content_orientation(tavros::ui::content_orient::horizontal);

        m_root_view.insert_last_child(&m_view1);
        m_root_view.add_child(&m_label);
        m_root_view.add_child(&m_label2);
        m_root_view.insert_last_child(&m_view2);
        m_root_view.insert_last_child(&m_horiz_view);
        m_root_view.insert_last_child(&m_view3);
        m_root_view.set_border(10);
        m_horiz_view.add_child(&m_label3);
        m_horiz_view.add_child(&m_label4);
        // m_root_view.insert_last_child(&m_view4);

        m_view1.set_size({500, 200});
        m_view1.set_margin({30, 60, 90, 120});
        m_view1.set_padding({40, 60, 80, 100});
        m_view1.set_vertical_alignment(tavros::ui::v_align::center);

        m_view2.set_size({200, 150});
        m_view2.set_margin({20, 10, 20, 10});
        m_view2.set_padding({20, 10, 30, 10});
        m_view2.set_vertical_alignment(tavros::ui::v_align::center);

        m_view3.set_size({100, 300});
        m_view3.set_margin({10, 10, 10, 10});
        m_view3.set_padding({50, 50, 50, 50});

        // m_view4.set_size({150, 250});
        // m_view4.set_margin({0, 10, 20, 0});
        // m_view4.set_padding({100, 0, 100, 00,});


        m_horiz_view.insert_last_child(&m_view5);
        m_horiz_view.insert_last_child(&m_view6);
        m_horiz_view.insert_last_child(&m_view7);

        m_horiz_view.set_margin({10, 10, 10, 10});

        m_view5.set_size({50, 50});
        m_view5.set_padding({10, 10, 10, 10});
        m_view5.set_vertical_alignment(tavros::ui::v_align::center);

        m_view6.set_margin({10, 20, 60, 80});
        m_view6.set_size({100, 200});
        m_view6.set_padding({20, 20, 20, 20});
        m_view6.set_vertical_alignment(tavros::ui::v_align::center);

        m_view7.set_size({300, 400});
        m_view7.set_margin({30, 30, 30, 30});

        m_label.set_font("fnt.consola_mono");
        m_label.set_font_size(48.0f);
        m_label.set_margin(45.0f);
        m_label.set_padding({125.0f, 25.0f, 125.0f, 25.0f});
        m_label.set_horizontal_alignment(tavros::ui::h_align::center);
        m_label.set_min_size({300.0f, 0});
        //m_label.set_max_size({400.0f, tavros::ui::k_max_ui_size});
        m_label.set_border(5);
        m_label.set_border_radius(10);
        m_label.set_word_wrap_enabled(true);
        m_label.set_size({150000.0f, 0});

        m_label2.set_font("fnt.noto_sans_regular");
        m_label2.set_font_size(44.0f);
        m_label2.set_text("Hello world!!!");
        m_label2.set_padding(10.0f);
        m_label2.set_margin(10.0f);
        m_label2.set_horizontal_alignment(tavros::ui::h_align::right);

        m_label3.set_font("fnt.home_video");
        m_label3.set_font_size(55.0f);
        m_label3.set_text("Label 3. Hello world!");
        m_label3.set_padding(10.0f);
        m_label3.set_margin(10.0f);

        m_label4.set_font("fnt.home_video");
        m_label4.set_font_size(55.0f);
        m_label4.set_text("Label 4. Hello label.");
        m_label4.set_padding(10.0f);
        m_label4.set_margin(10.0f);
        m_label4.set_horizontal_alignment(tavros::ui::h_align::center);
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
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_1)) {
            m_label.set_font("asdafadfasdfaerwtqgsafv");
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_2)) {
            m_label.set_font("fnt.droid_sans");
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_3)) {
            m_label.set_font("fnt.home_video");
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_4)) {
            m_label.set_font("fnt.noto_sans_regular");
        }

        if (m_input_manager.is_key_just_pressed(tavros::input::keyboard_key::k_5)) {
            m_label.set_font("fnt.roboto_medium");
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

        // depth в диапазоне [-1..1] -> alpha [0.2..1.0]
        auto depth_to_alpha = [](float depth) -> uint8 {
            if (depth > -0.2) {
                return 255;
            }
            float t = (depth + 1.2f);  // [-1..-0.2] -> [0..1]
            t = std::clamp(t, 0.0f, 1.0f);
            float a = 0.1f + t * 0.9f; // [0.2..1.0]
            return static_cast<uint8>(t * 255.0f);
        };

        auto with_alpha = [](math::rgba8 color, uint8 alpha) -> math::rgba8 {
            return {color.r, color.g, color.b, alpha};
        };

        constexpr auto x_color = math::rgba8(0xA73A53FF);
        constexpr auto y_color = math::rgba8(0x6BB023FF);
        constexpr auto z_color = math::rgba8(0x3A83BEFF);

        struct axis_info
        {
            math::vec2  dir;
            math::rgba8 color;
            float       depth;
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

        int order[3] = {0, 1, 2};
        std::sort(std::begin(order), std::end(order), [&](int a, int b) {
            return axes[a].depth > axes[b].depth;
        });

        auto* r = m_renderer->renderer2d();

        r->set_brush_solid_color(0x00000008);
        r->fill_circle(pos, radius);
        r->set_brush_solid_color(0x00000010);
        r->fill_ring(pos, radius, radius - line_width);

        r->set_line_thickness(line_width);

        tavros::renderer::rich_text text;
        text.set_font(m_font);
        r->set_text_use_brush_mask(false, false);

        auto draw_axis_neg = [&](const axis_info& ax) {
            // Негативный конец — глубина противоположна
            uint8 alpha = depth_to_alpha(-ax.depth);
            auto  col = with_alpha(ax.color, alpha);

            // Линия: градиент от центра (полный цвет) до конца (с альфой)
            /*r->set_brush_solid_color(col);
            r->set_line_dash(dash_size, gap_size);
            r->draw_line(pos, pos - ax.dir);
            r->set_line_dash(0.0f, 0.0f);*/

            // Круг и текст — solid с той же альфой
            r->set_brush_solid_color(col);
            r->fill_circle(pos - ax.dir, n_side_circle);

            text.clear();
            text.set_font_size(ax.neg_font_size);
            text.append_text(ax.neg_label);
            auto l = text.layout();
            r->set_brush_solid_color(col);
            r->draw_text(text.text(), pos - ax.dir - l.center(), 0.5f, 0.0f);
        };

        auto draw_axis_pos = [&](const axis_info& ax) {
            uint8 alpha = depth_to_alpha(ax.depth);
            auto  col = with_alpha(ax.color, alpha);

            // Линия: градиент от центра (полный цвет) до конца (с альфой)
            r->set_brush_solid_color(col);
            r->set_line_dash(0.0f, 0.0f);
            r->draw_line(pos, pos + ax.dir);

            // Круг и текст — solid с той же альфой
            r->set_brush_solid_color(col);
            r->fill_circle(pos + ax.dir, p_side_circle);

            text.clear();
            text.set_font_size(ax.pos_font_size);
            text.append_text(ax.pos_label);
            auto l = text.layout();
            r->set_brush_solid_color(col);
            r->draw_text(text.text(), pos + ax.dir - l.center(), 0.5f, 0.15f);
        };

        for (int i : order) {
            if (axes[i].depth > 0.0f) {
                draw_axis_neg(axes[i]);
            } else {
                draw_axis_pos(axes[i]);
            }
        }

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
    tavros::core::shared_ptr<tavros::renderer::render_system> m_renderer;

    // -- Scene --
    tavros::sandbox::free_camera m_free_cam;

    tavros::renderer::render_target_ref                 m_offscreen_rt;
    tavros::core::vector<tavros::renderer::texture_ref> m_sky_textures;
    int32                                               m_sky_index = 0;
    int32                                               m_smp_index = 0;

    tavros::renderer::texture_ref m_ui_texture;

    tavros::renderer::material_ref m_fullscreen_quad_pipeline;
    tavros::renderer::material_ref m_world_grid_pipeline;
    tavros::renderer::material_ref m_skybox_pipeline;

    tavros::renderer::font_ref m_font;

    // -- Input --
    tavros::input::input_manager m_input_manager;

    //
    uint64 m_frame_number = 0;

    tavros::core::unique_ptr<tavros::ui::root_view> m_ui;

    tavros::ui::linear_view m_root_view = tavros::ui::linear_view("root_view");
    tavros::ui::linear_view m_vert_view = tavros::ui::linear_view("vert_view");
    tavros::ui::linear_view m_horiz_view = tavros::ui::linear_view("horiz_view");
    tavros::ui::view m_view1 = tavros::ui::view("view1");
    tavros::ui::view m_view2 = tavros::ui::view("view2");
    tavros::ui::view m_view3 = tavros::ui::view("view3");
    tavros::ui::view m_view4 = tavros::ui::view("view4");
    tavros::ui::view m_view5 = tavros::ui::view("view5");
    tavros::ui::view m_view6 = tavros::ui::view("view6");
    tavros::ui::view m_view7 = tavros::ui::view("view7");
    tavros::ui::view m_view8 = tavros::ui::view("view8");
    tavros::ui::view m_view9 = tavros::ui::view("view9");
    tavros::ui::label_view m_label = tavros::ui::label_view("label");
    tavros::ui::label_view m_label2 = tavros::ui::label_view("label2");
    tavros::ui::label_view m_label3 = tavros::ui::label_view("label3");
    tavros::ui::label_view m_label4 = tavros::ui::label_view("label4");
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
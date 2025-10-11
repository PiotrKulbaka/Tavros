#include <stdio.h>

#include "render_app_base.hpp"
#include "image_decoder.hpp"

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/renderer/rhi/command_list.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/render_system.hpp>
#include <tavros/renderer/camera/camera.hpp>

#include <tavros/resources/resource_manager.hpp>
#include <tavros/resources/providers/filesystem_provider.hpp>
#include <tavros/core/memory/buffer.hpp>

#include <tavros/system/time.hpp>

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

class input_manager
{
public:
    input_manager()
    {
        clear_state();
    }

    /**
     * @brief Clears all key states.
     * Called for example when the window is deactivated,
     * to ensure that key states are valid when the window becomes active again.
     */
    void clear_state()
    {
        for (auto& state : m_keys) {
            state.press_time_us = 0;
            state.release_time_us = 0;
            state.accumulated_us = 0;
            state.is_pressed = false;
        }

        m_last_frame_time_us = 0;
        m_current_frame_time_us = 0;
        m_mouse_delta.set(0.0f, 0.0f);
    }

    /**
     * @brief Called at the beginning of each frame.
     * Updates time markers and resets accumulated frame-specific data.
     * @param frame_time_us start frame time
     */
    void on_frame_started(uint64 frame_time_us)
    {
        m_last_frame_time_us = m_current_frame_time_us;
        m_current_frame_time_us = frame_time_us;

        for (auto& state : m_keys) {
            state.accumulated_us = 0;
        }

        m_mouse_delta.set(0.0f, 0.0f);
    }

    /**
     * @brief Called when a key is pressed.
     * @param key Key identifier
     * @param time_us Time in microseconds when the event occurred
     */
    void on_key_press(tavros::system::keys key, uint64 time_us)
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);

        auto& s = m_keys[idx];
        if (!s.is_pressed) {
            s.is_pressed = true;
            s.press_time_us = time_us;
        }
    }

    /**
     * @brief Called when a key is released.
     * @param key Key identifier
     * @param time_us Time in microseconds when the event occurred
     */
    void on_key_release(tavros::system::keys key, uint64 time_us)
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);

        auto& s = m_keys[idx];
        if (s.is_pressed) {
            s.is_pressed = false;
            s.release_time_us = time_us;

            // Add a gap us if the key was pressed within the frame
            if (s.press_time_us > m_last_frame_time_us) {
                uint64 duration = std::min(time_us, m_current_frame_time_us) - s.press_time_us;
                s.accumulated_us += duration;
            }
        }
    }

    void on_mouse_move(tavros::math::vec2 delta, uint64 time_us)
    {
        TAV_UNUSED(time_us);

        m_mouse_delta += delta;
    }

    /**
     * @brief Returns how long the key was pressed during the last frame, in normalized [0..1] form.
     * @param key Key identifier
     * @return 0.0 if not pressed at all, 1.0 if pressed the entire frame, otherwise partial
     */
    double key_pressed_factor(tavros::system::keys key)
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);

        const auto& s = m_keys[idx];

        const uint64 frame_duration = m_current_frame_time_us - m_last_frame_time_us;
        if (frame_duration == 0) {
            return 0.0f;
        }

        uint64 total_us = s.accumulated_us;

        // If the key is pressed and still held, add the hold time to the current moment
        if (s.is_pressed) {
            uint64 pressed_since = std::max(s.press_time_us, m_last_frame_time_us);
            total_us += m_current_frame_time_us - pressed_since;
        }

        double factor = static_cast<double>(total_us) / static_cast<double>(frame_duration);
        return factor > 1.0 ? 1.0 : factor;
    }

    bool is_key_pressed(tavros::system::keys key)
    {
        const size_t idx = static_cast<size_t>(key);
        TAV_ASSERT(idx < k_keyboard_size);

        return m_keys[idx].is_pressed;
    }

    tavros::math::vec2 get_mouse_delta()
    {
        return m_mouse_delta;
    }

private:
    static constexpr size_t k_keyboard_size = static_cast<size_t>(tavros::system::keys::k_last_key);

    struct key_state
    {
        uint64 press_time_us = 0;   // Time when key was last pressed
        uint64 release_time_us = 0; // Time when key was last released
        uint64 accumulated_us = 0;  // Time accumulated during the current frame
        bool   is_pressed = false;  // Whether the key is currently pressed
    };

    uint64                                 m_current_frame_time_us = 0; // Time of the current frame
    uint64                                 m_last_frame_time_us = 0;    // Time of the previous frame
    std::array<key_state, k_keyboard_size> m_keys;                      // State data per key
    tavros::math::vec2                     m_mouse_delta;
};


class my_app : public app::render_app_base
{
public:
    my_app(tavros::core::shared_ptr<tavros::resources::resource_manager> resource_manager)
        : app::render_app_base("TavrosEngine")
        , m_image_decoder(&m_allocator)
        , m_resource_manager(resource_manager)
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

    app::image_decoder::pixels_view load_image(tavros::core::string_view path)
    {
        tavros::core::dynamic_buffer<uint8> buffer(&m_allocator);

        auto res = m_resource_manager->open(path);
        if (res) {
            auto* reader = res->reader();
            if (reader->is_open()) {
                auto size = reader->size();
                buffer.reserve(size);
                reader->read(buffer);
            }
        }

        // buffer.data() can be nullptr; decode_image will return fallback with white pixel
        return m_image_decoder.decode_image(buffer.data(), buffer.capacity());
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

        auto im_view = load_image("textures/base_wall/archpipe2_ifin.jpg");

        size_t tex_size = im_view.width * im_view.height * im_view.channels;
        auto   dst = m_graphics_device->map_buffer(m_stage_buffer);
        memcpy(dst.data(), im_view.data, tex_size);
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

    void process_events(app::event_queue_view events, double delta_time)
    {
        m_input_manager.on_frame_started(tavros::system::get_high_precision_system_time_us());

        bool need_resize = false;

        // Process all events
        for (auto& it : events) {
            switch (it.type) {
            case app::event_type::key_down:
                m_input_manager.on_key_press(it.key_info, it.event_time_us);
                break;

            case app::event_type::key_up:
                m_input_manager.on_key_release(it.key_info, it.event_time_us);
                break;

            case app::event_type::mouse_move:
                m_input_manager.on_mouse_move(it.vec_info, it.event_time_us);
                break;

            case app::event_type::mouse_button_down:
                break;

            case app::event_type::mouse_button_up:
                break;

            case app::event_type::window_resize:
                m_current_frame_size = tavros::math::ivec2(static_cast<int32>(it.vec_info.x), static_cast<int32>(it.vec_info.y));
                need_resize = true;
                break;

            case app::event_type::deactivate:
                m_input_manager.clear_state();
                break;

            case app::event_type::activate:
                m_input_manager.clear_state();
                break;

            default:
                break;
            }
        }

        if (need_resize) {
            m_composer->resize(m_current_frame_size.width, m_current_frame_size.height);
        }

        // Update camera
        auto factor = [&](tavros::system::keys key) -> float {
            return static_cast<float>(m_input_manager.key_pressed_factor(key));
        };

        tavros::math::vec3 move_delta =
            m_camera.forward() * factor(tavros::system::keys::k_W) - m_camera.forward() * factor(tavros::system::keys::k_S) + m_camera.right() * factor(tavros::system::keys::k_D) - m_camera.right() * factor(tavros::system::keys::k_A) + m_camera.up() * factor(tavros::system::keys::k_space) - m_camera.up() * factor(tavros::system::keys::k_C);

        float len = tavros::math::length(move_delta);
        if (len > 1.0f) {
            move_delta /= len;
        }

        m_camera.move(move_delta * static_cast<float>(delta_time) * 2.0f);

        auto mouse_delta = m_input_manager.get_mouse_delta();
        if (tavros::math::squared_length(mouse_delta) > 0.0f) {
            auto m = (mouse_delta / 5.0f) * static_cast<float>(delta_time);

            auto q_pitch = tavros::math::quat::from_axis_angle(m_camera.right(), -m.y);
            auto q_yaw = tavros::math::quat::from_axis_angle(tavros::math::vec3{0.0f, 1.0f, 0.0f}, -m.x);
            auto rotation = tavros::math::normalize(q_yaw * q_pitch);

            m_camera.set_orientation(rotation * m_camera.forward(), rotation * m_camera.up());
        }

        constexpr float fov_y = 60.0f * 3.14159265358979f / 180.0f; // 60 deg
        float           aspect_ratio = static_cast<float>(m_current_frame_size.width) / static_cast<float>(m_current_frame_size.height);
        m_camera.set_perspective(fov_y, aspect_ratio, 0.1f, 1000.0f);
    }

    void render(app::event_queue_view events, double delta_time) override
    {
        process_events(events, delta_time);

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

    app::image_decoder m_image_decoder;

    rhi::pipeline_handle       m_main_pipeline;
    rhi::render_pass_handle    m_main_pass;
    rhi::shader_binding_handle m_shader_binding;
    rhi::texture_handle        m_texture;
    rhi::buffer_handle         m_stage_buffer;
    rhi::sampler_handle        m_sampler;

    tavros::math::ivec2 m_current_frame_size;

    tavros::core::shared_ptr<tavros::resources::resource_manager> m_resource_manager;

    input_manager            m_input_manager;
    tavros::renderer::camera m_camera;
};

int main()
{
    tavros::core::logger::add_consumer([](auto lvl, auto tag, auto msg) {
        printf("%s\n", msg.data());
    });

    auto resource_manager = tavros::core::make_shared<tavros::resources::resource_manager>();
    resource_manager->mount<tavros::resources::filesystem_provider>("C:/Users/Piotr/Desktop/Tavros/assets");
    resource_manager->mount<tavros::resources::filesystem_provider>("C:/Work/q3pp_res/baseq3");

    auto app = std::make_unique<my_app>(resource_manager);
    return app->run();
}
#include "render_app_base.hpp"
#include "built_in_meshes.hpp"
#include "fps_meter.hpp"

#include "scene_data.hpp"
#include "text_rendering.hpp"
#include "pipeline_presets.hpp"
#include "particle_effectors.hpp"
#include "particle_emitter.hpp"
#include "particle_physics.hpp"
#include "particle_render.hpp"
#include "particle_shapes.hpp"
#include "thread_pool.hpp"
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
#include <tavros/core/resource/resource_registry.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/debug_renderer.hpp>
#include <tavros/renderer/render_target.hpp>
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
#include <tavros/renderer/mesh/mesh_data.hpp>
#include <tavros/renderer/rhi/string_utils.hpp>

#include <tavros/renderer/texture/texture_manager.hpp>

#include <tinyobjloader/tiny_obj_loader.h>

#include <glad/glad.h>
#include <tracy/Tracy.hpp>

#include <cstdio>
#include <cstdlib>
#include <unordered_map>

#include <tavros/core/config.hpp>


namespace tavros::sandbox
{
    constexpr auto x_axis_color = math::vec4(0.6549f, 0.2196f, 0.3255f, 1.0f);
    constexpr auto y_axis_color = math::vec4(0.4196f, 0.5569f, 0.1373f, 1.0f);
    constexpr auto z_axis_color = math::vec4(0.2314f, 0.5137f, 0.7412f, 1.0f);

} // namespace tavros::sandbox

constexpr uint32 k_msaa = 1;

namespace fs = tavros::filesystem;
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

// -------------------------------------------------------------------------
// AoS vertex layout used on the GPU side
// -------------------------------------------------------------------------

struct mesh_vertex
{
    tavros::math::vec3 pos;
    tavros::math::vec3 normal;
    tavros::math::vec2 uv;
};

// -------------------------------------------------------------------------
// Lightweight GPU mesh handle - offsets into shared vertex/index buffers
// -------------------------------------------------------------------------

struct gpu_mesh_slice
{
    uint32_t vertex_offset = 0; // first vertex in m_mesh_vertices_buffer (in vertices)
    uint32_t index_offset = 0;  // first index  in m_mesh_indices_buffer  (in indices)
    uint32_t index_count = 0;
};

// -------------------------------------------------------------------------
// OBJ -> mesh_data
// -------------------------------------------------------------------------

tavros::renderer::mesh_data load_obj_cpu(
    tavros::assets::asset_manager& am,
    tavros::core::string_view      path
)
{
    fs::fixed_path obj_path(path);
    obj_path.append(".obj");
    fs::fixed_path mtl_path(path);
    mtl_path.append(".mtl");


    const auto raw_obj = am.read_text(obj_path);
    const auto raw_mtl = am.read_text(mtl_path);

    tinyobj::ObjReaderConfig cfg;
    cfg.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromString(raw_obj, raw_mtl)) {
        g_logger.error("OBJ parse failed: {}", reader.Error());
        return {};
    }
    if (!reader.Warning().empty()) {
        g_logger.warning("OBJ warning: {}", reader.Warning());
    }

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();

    tavros::renderer::mesh_data mesh;

    // Deduplicate vertices using a flat key
    struct Key
    {
        int  vi, ni, ti;
        bool operator==(const Key& o) const noexcept
        {
            return vi == o.vi && ni == o.ni && ti == o.ti;
        }
    };
    struct KeyHash
    {
        size_t operator()(const Key& k) const noexcept
        {
            return static_cast<size_t>(k.vi) * 1000003u
                 ^ static_cast<size_t>(k.ni) * 999983u
                 ^ static_cast<size_t>(k.ti) * 999979u;
        }
    };
    std::unordered_map<Key, uint32_t, KeyHash> cache;

    for (const auto& shape : shapes) {
        for (const auto& idx : shape.mesh.indices) {
            Key key{idx.vertex_index, idx.normal_index, idx.texcoord_index};
            auto [it, inserted] = cache.emplace(key, static_cast<uint32_t>(mesh.vertex_count()));

            if (inserted) {
                tavros::renderer::position_c pos;
                pos.value = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 2],
                    attrib.vertices[3 * idx.vertex_index + 1] + 15.5f,
                };

                tavros::renderer::normal_c nrm;
                if (idx.normal_index >= 0) {
                    nrm.value = {
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2],
                    };
                    nrm.value = tavros::math::normalize(nrm.value);
                }

                tavros::renderer::uv0_c uv;
                if (idx.texcoord_index >= 0) {
                    uv.value = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * idx.texcoord_index + 1], // flip V
                    };
                }

                mesh.vertices.typed_emplace_back(pos, nrm, uv);
            }

            mesh.indices.push_back(it->second);
        }
    }

    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        auto tmp = mesh.indices[i + 1];
        mesh.indices[i + 1] = mesh.indices[i + 2];
        mesh.indices[i + 2] = tmp;
    }
    mesh.recompute_bounds();
    g_logger.info("OBJ loaded: {} verts, {} tris", mesh.vertex_count(), mesh.triangle_count());
    return mesh;
}

// -------------------------------------------------------------------------
// SoA mesh_data -> AoS interleaved vertex array
// -------------------------------------------------------------------------

tavros::core::vector<mesh_vertex> interleave_mesh(const tavros::renderer::mesh_data& mesh)
{
    const size_t                      n = mesh.vertex_count();
    tavros::core::vector<mesh_vertex> out(n);

    auto pos_view = mesh.vertices.get<tavros::renderer::position_c>();
    auto nrm_view = mesh.vertices.get<tavros::renderer::normal_c>();
    auto uv_view = mesh.vertices.get<tavros::renderer::uv0_c>();

    for (size_t i = 0; i < n; ++i) {
        out[i].pos = pos_view[i].value;
        out[i].normal = nrm_view[i].value;
        out[i].uv = uv_view[i].value;
    }

    return out;
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
        , m_font_lib(tavros::core::make_unique<filesystem_font_provider>(am))
        , m_thread_pool(std::thread::hardware_concurrency())
        , m_texture_manager(am)
    {
    }

    ~main_window() override = default;

    // ------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------

    void init() override
    {
        ZoneScopedNC("AppInit", 0x607D8B);
        init_graphics();
        m_texture_manager.init(m_graphics_device.get());
        init_offscreen();
        init_pipelines();
        init_scene_resources();
        init_fonts();
        init_ui();
        init_particle_pipeline();
        show();
        m_stage_buffer.reset();
    }

    void shutdown() override
    {
        ZoneScopedNC("AppShutdown", 0x607D8B);
        m_stage_buffer.shutdown();
        m_stream_draw.shutdown();
        m_drenderer.shutdown();
        m_offscreen_rt.shutdown();
        m_texture_manager.shutdown();
        m_graphics_device = nullptr;
    }

    // ------------------------------------------------------------------
    // Particle helpers (unchanged)
    // ------------------------------------------------------------------

    template<typename Rng>
    inline void add_explosion(
        tavros::particles::emitter_archetype& emitters,
        tavros::math::vec3 origin, float radius, Rng&& rng
    ) noexcept
    {
        using namespace tavros::particles;
        emitters.typed_emplace_back(emitter_c{
            .shape = spawn_shape::sphere_shell,
            .shape_params = {.center = origin, .radius = radius},
            .params = {
                .physics = k_spark_preset,
                .colors = {.stops = {
                               {0.0f, {255, 240, 50, 0}, {255, 255, 150, 0}},
                               {0.05f, {255, 100, 0, 255}, {255, 160, 30, 255}},
                               {0.6f, {180, 20, 0, 200}, {220, 50, 0, 220}},
                               {1.0f, {30, 30, 30, 0}, {60, 60, 60, 0}},
                           }},
                .velocity = {.speed_min = 1.0f, .speed_max = 4.0f, .spread_angle = 3.14159f},
                .lifetime_min = 12.8f,
                .lifetime_max = 22.5f,
                .size_start_min = 0.06f,
                .size_start_max = 0.22f,
                .size_end_min = 0.0f,
                .size_end_max = 0.0f,
                .avel_min = -6.0f,
                .avel_max = 6.0f,
            },
            .mode = emitter_mode::burst,
            .rate_min = 8000.0f,
            .rate_max = 12000.0f,
            .lifetime = 0.1f,
            .immortal = false,
        });
    }

    inline void add_portal(
        tavros::particles::emitter_archetype& emitters,
        tavros::math::vec3 center, float radius
    ) noexcept
    {
        using namespace tavros::particles;
        emitters.typed_emplace_back(emitter_c{
            .shape = spawn_shape::sphere_shell,
            .shape_params = {.center = center, .radius = radius},
            .params = {
                .physics = k_droplet_preset,
                .colors = {.stops = {
                               {0.0f, {100, 0, 255, 0}, {150, 50, 255, 0}},
                               {0.1f, {200, 0, 255, 200}, {0, 100, 255, 220}},
                               {0.8f, {50, 0, 180, 120}, {100, 0, 220, 150}},
                               {1.0f, {0, 0, 80, 0}, {20, 0, 100, 0}},
                           }},
                .velocity = {.speed_min = 0.15f, .speed_max = 1.0f, .direction = {0.0f, 0.0f, 0.0f}, .spread_angle = 3.14159f},
                .lifetime_min = 8.5f,
                .lifetime_max = 32.5f,
                .size_start_min = 0.03f,
                .size_start_max = 0.09f,
                .size_end_min = 0.0f,
                .size_end_max = 0.01f,
                .avel_min = -4.0f,
                .avel_max = 4.0f,
            },
            .mode = emitter_mode::burst,
            .rate_min = 3000.0f,
            .rate_max = 5000.0f,
            .immortal = false,
        });
    }

    void parallel_for(size_t n, size_t chunk_size, auto fn)
    {
        const size_t num_chunks = (n + chunk_size - 1) / chunk_size;
        TAV_ASSERT(num_chunks <= 64);
        for (size_t c = 0; c < num_chunks; ++c) {
            const size_t begin = c * chunk_size;
            const size_t count = std::min(chunk_size, n - begin);
            m_thread_pool.enqueue(fn, begin, count);
        }
        m_thread_pool.wait_all();
    }

    inline constexpr size_t calc_chunk_size(size_t n, size_t target_chunks, size_t min_chunk) noexcept
    {
        if (n == 0) {
            return min_chunk;
        }
        const size_t ideal = (n + target_chunks - 1) / target_chunks;
        const size_t pow2 = std::max(std::bit_floor(ideal), size_t{1});
        const size_t min_p2 = std::bit_floor(min_chunk);
        return std::max(pow2, min_p2);
    }

    // ------------------------------------------------------------------
    // Render loop
    // ------------------------------------------------------------------

    void render(tavros::input::event_args_queue_view events, double delta_time) override
    {
        ZoneScopedNC("Frame", 0xFFFFFF);

        if (m_frame_number % 3 == 0) {
            m_stage_buffer.reset();
            m_stream_draw.reset();
        }

        m_delta_time = static_cast<float>(delta_time);
        m_time = m_timer.elapsed_seconds();

        {
            ZoneScopedNC("ProcessInput", 0x9C27B0);
            process_input(events, delta_time);
            m_fps_meter.tick(delta_time);
        }
        {
            ZoneScopedNC("UpdateFrameData", 0x9C27B0);
            update_frame_data();
        }

        // ------------------------------------------------------------------
        // Simulation
        // ------------------------------------------------------------------
        {
            ZoneScopedNC("ParticlesClearDead", 0x2196F3);
            if (m_frame_number % 10 == 0) {
                clear_dead(m_particles);
            }
        }

        auto rng = [&](float lo, float hi) { return rand_range(lo, hi); };

        constexpr float spawn_area = 15;
        constexpr float max_strength = 5.0f;
        constexpr float min_strength = -2.0f;

        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_E)) {
            float x = rng(-spawn_area, spawn_area);
            float y = rng(-spawn_area, spawn_area);
            float z = rng(-spawn_area, spawn_area);
            m_particle_effectors.typed_emplace_back(
                tavros::particles::attractor_c{.origin = {x, y, z}, .strength = rng(min_strength, max_strength), .kill_radius = rng(0.1f, 1.0f)},
                tavros::particles::wind_c{{0.0f, 0.0f, 0.0f}}
            );
        }
        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_Q)) {
            const size_t sz = m_particle_effectors.size();
            if (sz > 0) {
                m_particle_effectors.swap_and_pop(rand() % sz);
            }
        }
        if (m_input_manager.is_key_held(tavros::input::keyboard_key::k_1)) {
            add_portal(m_particle_emitters, {10.0f, -10.0f, -10.0f}, 1.0f);
        }
        if (m_input_manager.is_key_held(tavros::input::keyboard_key::k_2)) {
            add_explosion(m_particle_emitters, {10.0f, 10.0f, 10.0f}, 2.0f, rng);
        }
        if (m_input_manager.is_key_held(tavros::input::keyboard_key::k_3)) {
            float x = rng(-spawn_area, spawn_area);
            float y = rng(-spawn_area, spawn_area);
            float z = rng(-spawn_area, spawn_area);
            add_portal(m_particle_emitters, {x, y, z}, rng(1.0f, 10.0f));
        }


        {
            ZoneScopedNC("EmittersUpdate", 0x2196F3);
            tavros::particles::update_emitters(m_particle_emitters, m_particles, delta_time, rng);
        }
        {
            ZoneScopedNC("ParticlesUpdate", 0x2196F3);
            const size_t n = m_particles.size();
            ZoneValue(static_cast<uint64_t>(n));
            const size_t chunk_size = calc_chunk_size(n, m_thread_pool.worker_count(), 256);
            {
                ZoneScopedNC("ClearForces", 0x2196F3);
                parallel_for(n, chunk_size, [&](size_t begin, size_t count) {
                    tavros::particles::clear_forces(m_particles, begin, count);
                });
            }
            {
                ZoneScopedNC("ApplyEffectors", 0x2196F3);
                parallel_for(n, chunk_size, [&](size_t begin, size_t count) {
                    tavros::particles::apply_effectors(m_particle_effectors, m_particles, begin, count);
                    tavros::particles::apply_drag(m_particles, begin, count);
                });
            }
            {
                ZoneScopedNC("Integrate", 0x2196F3);
                parallel_for(n, chunk_size, [&](size_t begin, size_t count) {
                    tavros::particles::integrate(m_particles, delta_time, begin, count);
                    tavros::particles::integrate_rotation(m_particles, delta_time, begin, count);
                    integrate_lt(m_particles, begin, count);
                });
            }
        }

        // ------------------------------------------------------------------
        // Render - CPU side
        // ------------------------------------------------------------------
        {
            ZoneScopedNC("DebugRendererBeginFrame", 0x4CAF50);
            m_drenderer.begin_frame(m_scene_data.ortho_projection, m_scene_data.view_projection);
            m_particle_effectors.view<tavros::particles::attractor_c>().each(
                [&](tavros::particles::attractor_c& a) {
                    float r = a.strength < 0.0f ? 0.0f : a.strength / max_strength;
                    float b = a.strength < 0.0f ? a.strength / min_strength : 0.0f;
                    m_drenderer.sphere3d({a.origin, a.kill_radius}, {r, 0.4f, b, 0.2f}, tavros::renderer::debug_renderer::draw_mode::faces);
                }
            );
        }

        auto* cbuf = m_composer->create_command_queue();


        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_4)) {
            m_mesh_texture = m_texture_manager.load(m_stage_buffer, *cbuf, "meshes/san_miguel/textures/piso_patio_exterior.png");
        }
        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_5)) {
            m_mesh_texture = m_texture_manager.load(m_stage_buffer, *cbuf, "meshes/san_miguel/textures/silla_d_piel.png");
        }
        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_6)) {
            m_mesh_texture = m_texture_manager.load(m_stage_buffer, *cbuf, "meshes/san_miguel/textures/Vigas_B.png");
        }
        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_7)) {
            m_mesh_texture = m_texture_manager.load(m_stage_buffer, *cbuf, "meshes/san_miguel/textures/BWK_1024.png");
        }
        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_U)) {
            // m_mesh_texture = {};
            // m_texture_manager.clear();
            m_texture_manager.release(m_mesh_texture);
        }
        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_P)) {
            for (auto [h, entry] : m_texture_manager) {
                auto& res = entry->res;
                g_logger.info("{}: w={}; h={}; ref_count: {} {}", h, res.width, res.height, entry->rc.load(), entry->key);
            }
        }

        {
            ZoneScopedNC("UploadFrameUniforms", 0xFF9800);
            upload_frame_uniforms();
        }
        {
            ZoneScopedNC("BuildFpsText", 0x00BCD4);
            build_fps_text();
        }

        auto glyph_slice = m_stream_draw.slice<app::glyph_instance>(m_fps_text.size());
        auto glyph_count = app::fill_glyph_instances(m_fps_text, glyph_slice.data(), {16.0f, 16.0f});


        auto particle_slice = [&] {
            ZoneScopedNC("PrepareParticleSlice", 0xFF9800);
            return prepare_particle_slice();
        }();

        {
            ZoneScopedNC("BeginFrame", 0x4CAF50);
            m_composer->begin_frame();
        }

        {
            ZoneScopedNC("RecordOffscreenPass", 0x4CAF50);
            record_offscreen_pass(cbuf, glyph_slice, glyph_count, particle_slice);
        }

        if (m_capture_next_frame) {
            ZoneScopedNC("CaptureFrame", 0x795548);
            capture_frame(cbuf);
        }

        {
            ZoneScopedNC("RecordBlitToBackbuffer", 0x4CAF50);
            record_blit_to_backbuffer(cbuf);
        }

        {
            ZoneScopedNC("SubmitCommandQueue", 0xFF9800);
            m_composer->submit_command_queue(cbuf);
            m_composer->end_frame();
        }
        {
            ZoneScopedNC("WaitForFrameComplete", 0xF44336);
            m_composer->wait_for_frame_complete();
        }
        {
            ZoneScopedNC("Present", 0xF44336);
            m_composer->present();
        }

        if (m_capture_next_frame) {
            ZoneScopedNC("SaveFrameCapture", 0x795548);
            save_frame_capture();
            m_capture_next_frame = false;
        }

        TracyPlotConfig("Particles", tracy::PlotFormatType::Number, true, true, 0x2196F3);
        TracyPlot("Particles", static_cast<int64_t>(m_particles.size()));
        TracyPlotConfig("FPS", tracy::PlotFormatType::Number, false, true, 0x4CAF50);
        TracyPlot("FPS", static_cast<double>(m_fps_meter.average_fps()));

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

        m_graphics_device = rhi::graphics_device::create(rhi::render_backend_type::opengl);
        TAV_FATAL_IF(!m_graphics_device, "Failed to create graphics device.");

        rhi::frame_composer_create_info info;
        info.width = 1;
        info.height = 1;
        info.buffer_count = 2;
        info.vsync = false;
        info.color_attachment_format = rhi::pixel_format::rgba8un;
        info.depth_stencil_attachment_format = rhi::pixel_format::depth24_stencil8;
        info.native_handle = native_handle();

        auto handle = m_graphics_device->create_frame_composer(info);
        TAV_FATAL_IF(!handle, "Failed to create frame composer.");
        m_composer = m_graphics_device->get_frame_composer_ptr(handle);
        TAV_FATAL_IF(!m_composer, "Failed to get frame composer pointer.");

        m_fence = m_graphics_device->create_fence();
        TAV_FATAL_IF(!m_fence, "Failed to create fence.");

        m_stage_buffer.init(m_graphics_device.get(), 256_MiB);
        m_stage_upload_buffer = create_buffer(64_MiB, rhi::buffer_usage::stage, rhi::buffer_access::gpu_to_cpu);
        m_uniform_buffer = create_buffer(1_MiB, rhi::buffer_usage::storage, rhi::buffer_access::gpu_only);
        m_stream_draw.init(m_graphics_device.get(), 256_MiB, rhi::buffer_usage::vertex);

        m_free_cam.set_orientation(tavros::math::quat::look_rotation(tavros::math::vec3(-1.0f, -1.0f, -0.5f), tavros::math::vec3(0.0f, 0.0f, 1.0f)));
    }

    void init_offscreen()
    {
        ZoneScopedNC("InitOffscreen", 0x607D8B);
        m_offscreen_rt.init(m_graphics_device.get(), rhi::pixel_format::rgba8un, rhi::pixel_format::depth32f);

        rhi::render_pass_create_info rp;
        rp.color_attachments.push_back({rhi::pixel_format::rgba8un, rhi::load_op::clear, rhi::store_op::dont_care, {}, {0.2f, 0.2f, 0.25f, 1.0f}});
        rp.depth_stencil_attachment.format = rhi::pixel_format::depth24_stencil8;
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
        ZoneScopedNC("InitPipelines", 0x607D8B);
        m_fullscreen_quad_pipeline = build_fullscreen_quad_pipeline();
        m_mesh_rendering_pipeline = build_mesh_pipeline();
        m_world_grid_pipeline = build_world_grid_pipeline();
        m_sdf_font_pipeline = build_sdf_font_pipeline();
        m_skybox_pipeline = build_skybox_pipeline();
        m_line2d_pipeline = build_line2d_pipeline();
        m_circle2d_pipeline = build_circle2d_pipeline();
        m_sprite3d_pipeline = build_sprite3d_pipeline();
    }

    void init_scene_resources()
    {
        ZoneScopedNC("InitSceneResources", 0x607D8B);

        auto* cmd = m_composer->create_command_queue();
        m_mesh_texture = m_texture_manager.load(m_stage_buffer, *cmd, "textures/cube_test.png");
        tavros::renderer::texture_manager::load_params tex_params;
        tex_params.array_layers = 13;
        m_boom_texture = m_texture_manager.load(m_stage_buffer, *cmd, "textures/boom.png", tex_params);
        tavros::renderer::texture_manager::load_params cubemap_params;
        cubemap_params.type = rhi::texture_type::texture_cube;
        m_skybox_texture = m_texture_manager.load(m_stage_buffer, *cmd, "textures/cubemaps/puresky.png", cubemap_params);
        m_composer->submit_command_queue(cmd);

        rhi::sampler_create_info ssi;
        ssi.filter.mipmap_filter = rhi::mipmap_filter_mode::linear;
        ssi.filter.min_filter = rhi::filter_mode::linear;
        ssi.filter.mag_filter = rhi::filter_mode::linear;
        ssi.wrap_mode.wrap_s = rhi::wrap_mode::clamp_to_edge;
        ssi.wrap_mode.wrap_t = rhi::wrap_mode::clamp_to_edge;
        ssi.wrap_mode.wrap_r = rhi::wrap_mode::clamp_to_edge;
        m_skybox_sampler = m_graphics_device->create_sampler(ssi);
        TAV_FATAL_IF(!m_skybox_sampler, "Failed to create sampler.");

        // Sampler
        rhi::sampler_create_info si;
        si.filter.mipmap_filter = rhi::mipmap_filter_mode::linear;
        si.filter.min_filter = rhi::filter_mode::linear;
        si.filter.mag_filter = rhi::filter_mode::linear;
        m_sampler = m_graphics_device->create_sampler(si);
        TAV_FATAL_IF(!m_sampler, "Failed to create sampler.");

        // ------------------------------------------------------------------
        // OBJ mesh - loaded into the shared vertex/index buffers
        // ------------------------------------------------------------------
        // Buffer sizes: 16 MiB vertices, 128 KiB indices - same as before.
        // These are reused; one mesh occupies the whole buffer for now.
        m_mesh_vertices_buffer = create_buffer(256_MiB, rhi::buffer_usage::vertex, rhi::buffer_access::cpu_to_gpu);
        m_mesh_indices_buffer = create_buffer(128_MiB, rhi::buffer_usage::index, rhi::buffer_access::cpu_to_gpu);

        load_and_upload_obj("meshes/sibenik/sibenik");
    }

    // Load OBJ, convert to AoS, upload into the shared buffers.
    // Fills m_obj_mesh with offsets / count.
    void load_and_upload_obj(tavros::core::string_view path)
    {
        auto cpu_mesh = load_obj_cpu(*m_am.get(), path);
        if (cpu_mesh.empty()) {
            g_logger.error("OBJ mesh is empty, nothing to upload.");
            return;
        }

        // Convert SoA -> AoS
        auto verts = interleave_mesh(cpu_mesh);

        const size_t vb_bytes = verts.size() * sizeof(mesh_vertex);
        const size_t ib_bytes = cpu_mesh.index_count() * sizeof(uint32_t);

        TAV_FATAL_IF(vb_bytes > 256_MiB, "OBJ vertex data exceeds vertex buffer size.");
        TAV_FATAL_IF(ib_bytes > 128_MiB, "OBJ index data exceeds index buffer size.");

        upload_to_mapped(m_mesh_vertices_buffer, verts.data(), vb_bytes);
        upload_to_mapped(m_mesh_indices_buffer, cpu_mesh.indices.data(), ib_bytes);

        m_obj_mesh.vertex_offset = 0;
        m_obj_mesh.index_offset = 0;
        m_obj_mesh.index_count = static_cast<uint32_t>(cpu_mesh.index_count());
    }

    void init_fonts()
    {
        ZoneScopedNC("InitFonts", 0x607D8B);
        m_font_lib.load("fonts/Consola-Mono.ttf", "Consola-Mono");
        m_font_lib.load("fonts/DroidSans.ttf", "DroidSans");
        m_font_lib.load("fonts/HomeVideo.ttf", "HomeVideo");
        m_font_lib.load("fonts/NotoSans-Regular.ttf", "NotoSans-Regular");
        m_font_lib.load("fonts/Roboto-Medium.ttf", "Roboto-Medium");
        m_font_texture = bake_and_upload_font_atlas();
    }

    void init_ui()
    {
        ZoneScopedNC("InitUi", 0x607D8B);
        m_drenderer.init(m_graphics_device.get());
    }

    void init_particle_pipeline()
    {
        ZoneScopedNC("InitParticlePipeline", 0x607D8B);
        using namespace app::pipeline_builder;

        auto vert = load_and_create_shader("tavros/shaders/particle.vert", rhi::shader_stage::vertex);
        auto frag = load_and_create_shader("tavros/shaders/particle.frag", rhi::shader_stage::fragment);

        rhi::pipeline_create_info info;
        using tavros::particles::particle_instance;
        info.bindings.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0, sizeof(particle_instance), offsetof(particle_instance, position), 1});
        info.bindings.push_back({rhi::attribute_type::scalar, rhi::attribute_format::f32, false, 1, sizeof(particle_instance), offsetof(particle_instance, size), 1});
        info.bindings.push_back({rhi::attribute_type::scalar, rhi::attribute_format::u32, false, 2, sizeof(particle_instance), offsetof(particle_instance, color), 1});
        info.bindings.push_back({rhi::attribute_type::scalar, rhi::attribute_format::f32, false, 3, sizeof(particle_instance), offsetof(particle_instance, rotation), 1});
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
        ZoneScopedNC("InputProcessEvents", 0x9C27B0);
        m_input_manager.begin_frame(tavros::system::application::instance().highp_time_us());
        m_input_manager.process_events(events);
        handle_key_shortcuts();
        handle_window_resize();
        static bool fly_cam = false;
        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_equal)) {
            fly_cam = !fly_cam;
            m_free_cam.set_fly(fly_cam);
        }
        m_free_cam.update(m_input_manager, delta_time);
    }

    void handle_key_shortcuts()
    {
        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_F1)) {
            m_current_buffer_output = 0;
        }
        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_F2)) {
            m_current_buffer_output = 1;
        }
        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_F10)) {
            m_capture_next_frame = true;
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
        m_offscreen_rt.resize(static_cast<uint32>(sz.width), static_cast<uint32>(sz.height), k_msaa);
        m_composer->resize(sz.width, sz.height);
        constexpr float k_fov_y = 60.0f * 3.14159265358979f / 180.0f;
        m_free_cam.set_perspective(k_fov_y, static_cast<float>(sz.width) / static_cast<float>(sz.height), 0.1f, 5000.0f);
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
        m_scene_data.time = m_time;
        m_scene_data.delta_time = m_delta_time;
        m_scene_data.frame_index = static_cast<uint32>(m_frame_number);
        m_scene_data._pad1 = 0.0f;
    }

    void upload_frame_uniforms()
    {
        m_stage_buffer.reset();
        auto slice = m_stage_buffer.slice<app::scene_data>(1);

        slice.data().copy_from(&m_scene_data, 1);
        m_stage_to_scene_offset = slice.offset_bytes();
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
        label("\nMesh verts: ");
        value(m_obj_mesh.index_count / 3 * 3); // triangles*3

        tavros::renderer::text_layouter::layout(m_fps_text);
    }

    struct particle_draw_slice
    {
        tavros::renderer::gpu_buffer_view<tavros::particles::particle_instance> slice{};
        size_t                                                                  count = 0;
    };

    particle_draw_slice prepare_particle_slice()
    {
        if (m_particles.size() == 0) {
            return {};
        }

        particle_draw_slice result;
        result.slice = m_stream_draw.slice<tavros::particles::particle_instance>(m_particles.size());
        const size_t        n = m_particles.size();
        const size_t        chunk_size = calc_chunk_size(n, m_thread_pool.worker_count(), 1024);
        std::atomic<size_t> total_count{0};

        parallel_for(n, chunk_size, [&](size_t begin, size_t count) {
            tavros::particles::fill_instances(m_particles, result.slice.data(), begin, count);
            total_count.fetch_add(count, std::memory_order_relaxed);
        });

        result.count = total_count.load();
        return result;
    }

    uint32 m_plane_grid_vb = 0;

    void record_offscreen_pass(
        rhi::command_queue*                                           cbuf,
        const tavros::renderer::gpu_buffer_view<app::glyph_instance>& glyph_slice,
        size_t                                                        glyph_count,
        const particle_draw_slice&                                    pslice
    )
    {
        ZoneScopedNC("OffscreenPass", 0x4CAF50);

        constexpr size_t scene_size = sizeof(app::scene_data);
        cbuf->copy_buffer(m_stage_buffer.gpu_buffer(), m_uniform_buffer, scene_size, m_stage_to_scene_offset, m_stage_to_scene_offset);
        m_scene_binding = rhi::buffer_binding{
            m_uniform_buffer,
            static_cast<uint32>(m_stage_to_scene_offset),
            scene_size, 0
        };

        cbuf->begin_render_pass(m_offscreen_rt.render_pass(), m_offscreen_rt.framebuffer());
        cbuf->bind_shader_buffers(m_scene_binding);

        {
            ZoneScopedNC("DrawMesh", 0x4CAF50);
            // draw_mesh(cbuf);
        }

        draw_skybox(cbuf);

        if (m_input_manager.is_key_just_released(tavros::input::keyboard_key::k_minus)) {
            m_plane_grid_vb++;
            if (m_plane_grid_vb >= 4) {
                m_plane_grid_vb = 0;
            }
        }

        {
            ZoneScopedNC("DrawWorldGrid", 0x4CAF50);
            if (m_plane_grid_vb <= 2) {
                draw_world_grid(*cbuf, m_plane_grid_vb);
            }
        }
        {
            ZoneScopedNC("DrawHudText", 0x00BCD4);
            draw_hud_text(cbuf, glyph_slice, glyph_count);
        }
        {
            ZoneScopedNC("DrawParticles", 0x2196F3);
            draw_particles(cbuf, pslice);
        }

        draw_boom(cbuf, {0.0f, 0.0f, 0.0f}, 2.0f, (m_frame_number / 3) % 16, static_cast<float>(m_frame_number) / 100.0f);

        float w = m_composer->width();
        draw_world_axis(*cbuf, {w - 120.0f, 120.0f}, 100.0f);

        {
            ZoneScopedNC("DebugRendererFlush", 0x4CAF50);
            m_drenderer.update();
            m_drenderer.render(cbuf);
            m_drenderer.end_frame();
        }

        cbuf->end_render_pass();
    }

    void draw_mesh(rhi::command_queue* cbuf)
    {
        if (m_obj_mesh.index_count == 0) {
            return;
        }

        cbuf->bind_pipeline(m_mesh_rendering_pipeline);

        // Single interleaved buffer bound to all three attribute slots
        rhi::bind_buffer_info bufs[] = {
            {m_mesh_vertices_buffer, m_obj_mesh.vertex_offset * sizeof(mesh_vertex)},
            {m_mesh_vertices_buffer, m_obj_mesh.vertex_offset * sizeof(mesh_vertex)},
            {m_mesh_vertices_buffer, m_obj_mesh.vertex_offset * sizeof(mesh_vertex)},
        };
        cbuf->bind_vertex_buffers(bufs);
        cbuf->bind_index_buffer(m_mesh_indices_buffer, rhi::index_buffer_format::u32);
        cbuf->bind_shader_buffers(m_scene_binding);
        cbuf->bind_shader_textures(rhi::texture_binding{m_texture_manager.get_gpu_handle(m_mesh_texture), m_sampler});
        cbuf->draw_indexed(m_obj_mesh.index_count, m_obj_mesh.index_offset, 0 /* base vertex */);
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

        auto plane_id = plane & 0x3;
        if (plane_id == 0) { // XY plane
            u_color = math::rgba8(sandbox::x_axis_color).color;
            v_color = math::rgba8(sandbox::y_axis_color).color;
        } else if (plane_id == 1) { // YZ plane
            u_color = math::rgba8(sandbox::y_axis_color).color;
            v_color = math::rgba8(sandbox::z_axis_color).color;
        } else if (plane_id == 2) { // ZX plane
            u_color = math::rgba8(sandbox::z_axis_color).color;
            v_color = math::rgba8(sandbox::x_axis_color).color;
        }

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

    void draw_skybox(rhi::command_queue* cbuf)
    {
        cbuf->bind_pipeline(m_skybox_pipeline);
        cbuf->bind_shader_textures(rhi::texture_binding{m_texture_manager.get_gpu_handle(m_skybox_texture), m_skybox_sampler, 0});
        cbuf->draw(36);
    }

    void draw_boom(rhi::command_queue* cbuf, const tavros::math::vec3& pos, float size, uint32 layer, float rot)
    {
        struct push_data
        {
            tavros::math::vec3 pos;
            float              _pad0 = 0.0f;
            tavros::math::vec2 size;
            tavros::math::vec2 _pad1;
            tavros::math::vec4 color;
            float              layer = 0.0f;
            float              rotation = 0.0f;
        };

        push_data pc;
        pc.pos = pos;
        pc.size = tavros::math::vec2(size);
        pc.color = {1.0f, 1.0f, 1.0f, 1.0f};
        pc.layer = static_cast<float>(layer);
        pc.rotation = rot;
        cbuf->bind_pipeline(m_sprite3d_pipeline);
        cbuf->push_constant(pc);
        cbuf->bind_shader_textures(rhi::texture_binding{m_texture_manager.get_gpu_handle(m_boom_texture), m_sampler, 0});
        cbuf->draw(6);
    }

    void draw_line_2d(rhi::command_queue& cmd, tavros::math::vec2 p0, tavros::math::vec2 p1, tavros::math::vec4 color, float dash_size = 0.0f, float gap_size = 0.0f, float thickness = 4.0f, float aa_width = 1.2f)
    {
        struct push_data
        {
            tavros::math::vec2 p0;
            tavros::math::vec2 p1;
            tavros::math::vec4 color;
            float              thickness = 0.0f;
            float              aa_width = 0.0f;
            float              dash_size = 0.0f;
            float              gap_size = 0.0f;
        };

        push_data pc;
        pc.p0 = p0;
        pc.p1 = p1;
        pc.color = color;
        pc.thickness = thickness;
        pc.aa_width = aa_width;
        pc.dash_size = dash_size;
        pc.gap_size = gap_size;

        cmd.push_constant(pc);
        cmd.draw(6);
    }

    void draw_circle_2d(rhi::command_queue& cmd, tavros::math::vec2 p, float out_r, float in_r, tavros::math::vec4 color, float dash_size = 0.0f, float gap_size = 0.0f, float aa_width = 1.2f)
    {
        struct push_data
        {
            tavros::math::vec2 pos;
            float              out_r = 0.0f;
            float              in_r = 0.0f;
            tavros::math::vec4 color;
            float              aa_width = 0.0f;
            float              dash_size = 0.0f;
            float              gap_size = 0.0f;
        };

        push_data pc;
        pc.pos = p;
        pc.out_r = out_r;
        pc.in_r = in_r;
        pc.color = color;
        pc.aa_width = aa_width;
        pc.dash_size = dash_size;
        pc.gap_size = gap_size;

        cmd.push_constant(pc);
        cmd.draw(6);
    }

    void draw_world_axis(rhi::command_queue& cbuf, const tavros::math::vec2& pos, float r)
    {
        using namespace tavros;

        float line_width = 6.0f;
        float dash_size = 10.0f;
        float gap_size = 5.0f;
        float p_side_circle = 14.0f;
        float n_side_circle = 8.0f;

        float axis_len = r - p_side_circle - line_width;

        auto view3 = math::mat3(m_free_cam.camera().view_matrix());

        auto project_axis = [&](math::vec3 world_dir) -> math::vec2 {
            math::vec3 cam = view3 * world_dir;
            return math::vec2(cam.x, -cam.y) * axis_len;
        };

        auto x_axis = project_axis({1.0f, 0.0f, 0.0f});
        auto y_axis = project_axis({0.0f, 1.0f, 0.0f});
        auto z_axis = project_axis({0.0f, 0.0f, 1.0f});

        cbuf.bind_pipeline(m_circle2d_pipeline);
        draw_circle_2d(cbuf, pos, r, 0.0f, {0.9f, 0.9f, 0.9f, 0.25f});
        draw_circle_2d(cbuf, pos, r, r - line_width, {0.75f, 0.75f, 0.75f, 0.45f});

        cbuf.bind_pipeline(m_line2d_pipeline);
        draw_line_2d(cbuf, pos, pos + x_axis, sandbox::x_axis_color, 0.0f, 0.0f, line_width);
        draw_line_2d(cbuf, pos, pos - x_axis, sandbox::x_axis_color, dash_size, gap_size, line_width);
        draw_line_2d(cbuf, pos, pos + y_axis, sandbox::y_axis_color, 0.0f, 0.0f, line_width);
        draw_line_2d(cbuf, pos, pos - y_axis, sandbox::y_axis_color, dash_size, gap_size, line_width);
        draw_line_2d(cbuf, pos, pos + z_axis, sandbox::z_axis_color, 0.0f, 0.0f, line_width);
        draw_line_2d(cbuf, pos, pos - z_axis, sandbox::z_axis_color, dash_size, gap_size, line_width);

        cbuf.bind_pipeline(m_circle2d_pipeline);
        draw_circle_2d(cbuf, pos + x_axis, p_side_circle, 0.0f, sandbox::x_axis_color);
        draw_circle_2d(cbuf, pos + y_axis, p_side_circle, 0.0f, sandbox::y_axis_color);
        draw_circle_2d(cbuf, pos + z_axis, p_side_circle, 0.0f, sandbox::z_axis_color);
        draw_circle_2d(cbuf, pos - x_axis, n_side_circle, 0.0f, sandbox::x_axis_color);
        draw_circle_2d(cbuf, pos - y_axis, n_side_circle, 0.0f, sandbox::y_axis_color);
        draw_circle_2d(cbuf, pos - z_axis, n_side_circle, 0.0f, sandbox::z_axis_color);
    }

    void draw_hud_text(
        rhi::command_queue*                                           cbuf,
        const tavros::renderer::gpu_buffer_view<app::glyph_instance>& slice,
        size_t                                                        count
    )
    {
        cbuf->bind_pipeline(m_sdf_font_pipeline);
        auto* tex = m_texture_manager.get(m_font_texture);
        cbuf->bind_shader_textures(rhi::texture_binding{tex->gpu_handle, m_sampler, 0});
        cbuf->bind_shader_buffers(m_scene_binding);

        rhi::bind_buffer_info bufs[] = {
            {slice.gpu_buffer(), slice.offset_bytes()},
            {slice.gpu_buffer(), slice.offset_bytes()},
            {slice.gpu_buffer(), slice.offset_bytes()},
            {slice.gpu_buffer(), slice.offset_bytes()},
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
        cbuf->bind_shader_buffers(m_scene_binding);

        rhi::bind_buffer_info bufs[] = {
            {pslice.slice.gpu_buffer(), pslice.slice.offset_bytes()},
            {pslice.slice.gpu_buffer(), pslice.slice.offset_bytes()},
            {pslice.slice.gpu_buffer(), pslice.slice.offset_bytes()},
            {pslice.slice.gpu_buffer(), pslice.slice.offset_bytes()},
        };
        cbuf->bind_vertex_buffers(bufs);
        cbuf->draw(4, 0, static_cast<uint32>(pslice.count), 0);
    }

    void record_blit_to_backbuffer(rhi::command_queue* cbuf)
    {
        ZoneScopedNC("BlitToBackbuffer", 0x4CAF50);
        cbuf->begin_render_pass(m_main_pass, m_composer->backbuffer());
        cbuf->bind_pipeline(m_fullscreen_quad_pipeline);
        if (m_current_buffer_output == 0) {
            cbuf->bind_shader_textures(rhi::texture_binding{m_offscreen_rt.color_attachment(0), m_sampler});
        } else {
            cbuf->bind_shader_textures(rhi::texture_binding{m_offscreen_rt.depth_stencil_attachment(), m_sampler});
        }
        cbuf->draw(4);
        cbuf->end_render_pass();
    }

    void capture_frame(rhi::command_queue* cbuf)
    {
        const int w = m_input_manager.window_size().width;
        const int h = m_input_manager.window_size().height;

        rhi::texture_copy_region rgn;
        rgn.width = w;
        rgn.height = h;
        cbuf->copy_texture_to_buffer(m_offscreen_rt.color_attachment(0), m_stage_upload_buffer, rgn);

        rgn.buffer_offset = static_cast<size_t>(w * h * 4);
        cbuf->copy_texture_to_buffer(m_offscreen_rt.depth_stencil_attachment(), m_stage_upload_buffer, rgn);
    }

    void save_frame_capture()
    {
        const int w = m_input_manager.window_size().width;
        const int h = m_input_manager.window_size().height;

        auto         map = m_graphics_device->map_buffer(m_stage_upload_buffer);
        const size_t plane_size = static_cast<size_t>(w * h);

        tavros::assets::image_view color_im(
            {map.data(), plane_size * 4}, w, h, tavros::assets::image::pixel_format::rgba8
        );
        save_image(color_im, "output://color.png", true);

        tavros::core::dynamic_buffer<uint8> depth_buf(plane_size);
        const float*                        src = reinterpret_cast<const float*>(map.data() + plane_size * 4);
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
        // AoS layout: all three attributes from the same buffer, different offsets/strides
        info.bindings.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0, sizeof(mesh_vertex), offsetof(mesh_vertex, pos), 0});
        info.bindings.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 1, sizeof(mesh_vertex), offsetof(mesh_vertex, normal), 0});
        info.bindings.push_back({rhi::attribute_type::vec2, rhi::attribute_format::f32, false, 2, sizeof(mesh_vertex), offsetof(mesh_vertex, uv), 0});
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

    rhi::pipeline_handle build_skybox_pipeline()
    {
        using namespace app::pipeline_builder;
        auto vert = load_and_create_shader("tavros/shaders/skybox.vert", rhi::shader_stage::vertex);
        auto frag = load_and_create_shader("tavros/shaders/skybox.frag", rhi::shader_stage::fragment);

        rhi::pipeline_create_info info;
        info.shaders.push_back(vert);
        info.shaders.push_back(frag);
        info.depth_stencil = depth_test_leq();
        info.rasterizer = cull_off();
        info.topology = rhi::primitive_topology::triangles;
        info.blend_states.push_back(no_blend());
        info.multisample = no_msaa();

        auto h = m_graphics_device->create_pipeline(info);
        TAV_FATAL_IF(!h, "Failed to create Skybox pipeline.");
        m_graphics_device->destroy_shader(vert);
        m_graphics_device->destroy_shader(frag);
        return h;
    }

    rhi::pipeline_handle build_line2d_pipeline()
    {
        using namespace app::pipeline_builder;

        auto vert = load_and_create_shader("tavros/shaders/line2d.vert", rhi::shader_stage::vertex);
        auto frag = load_and_create_shader("tavros/shaders/line2d.frag", rhi::shader_stage::fragment);

        rhi::pipeline_create_info info;
        info.shaders.push_back(vert);
        info.shaders.push_back(frag);
        info.depth_stencil = depth_off();
        info.rasterizer = cull_off();
        info.topology = rhi::primitive_topology::triangles;
        info.blend_states.push_back(alpha_blend());
        info.multisample = no_msaa();

        auto h = m_graphics_device->create_pipeline(info);
        TAV_FATAL_IF(!h, "Failed to create line2d pipeline.");
        m_graphics_device->destroy_shader(vert);
        m_graphics_device->destroy_shader(frag);
        return h;
    }


    rhi::pipeline_handle build_circle2d_pipeline()
    {
        using namespace app::pipeline_builder;

        auto vert = load_and_create_shader("tavros/shaders/circle2d.vert", rhi::shader_stage::vertex);
        auto frag = load_and_create_shader("tavros/shaders/circle2d.frag", rhi::shader_stage::fragment);

        rhi::pipeline_create_info info;
        info.shaders.push_back(vert);
        info.shaders.push_back(frag);
        info.depth_stencil = depth_off();
        info.rasterizer = cull_off();
        info.topology = rhi::primitive_topology::triangles;
        info.blend_states.push_back(alpha_blend());
        info.multisample = no_msaa();

        auto h = m_graphics_device->create_pipeline(info);
        TAV_FATAL_IF(!h, "Failed to create circle2d pipeline.");
        m_graphics_device->destroy_shader(vert);
        m_graphics_device->destroy_shader(frag);
        return h;
    }

    rhi::pipeline_handle build_sprite3d_pipeline()
    {
        using namespace app::pipeline_builder;

        auto vert = load_and_create_shader("tavros/shaders/sprite3d.vert", rhi::shader_stage::vertex);
        auto frag = load_and_create_shader("tavros/shaders/sprite3d.frag", rhi::shader_stage::fragment);

        rhi::pipeline_create_info info;
        info.shaders.push_back(vert);
        info.shaders.push_back(frag);
        info.depth_stencil = depth_on_write_off();
        info.rasterizer = cull_off();
        info.topology = rhi::primitive_topology::triangles;
        info.blend_states.push_back(alpha_blend());
        info.multisample = no_msaa();

        auto h = m_graphics_device->create_pipeline(info);
        TAV_FATAL_IF(!h, "Failed to create sprite3d pipeline.");
        m_graphics_device->destroy_shader(vert);
        m_graphics_device->destroy_shader(frag);
        return h;
    }

    // ==================================================================
    // Resource helpers
    // ==================================================================

    rhi::buffer_handle create_buffer(size_t size, rhi::buffer_usage usage, rhi::buffer_access access)
    {
        rhi::buffer_create_info info{size, usage, access};
        auto                    h = m_graphics_device->create_buffer(info);
        TAV_FATAL_IF(!h, "Failed to create buffer");
        return h;
    }

    void upload_to_mapped(rhi::buffer_handle dst, const void* data, size_t size)
    {
        auto map = m_graphics_device->map_buffer(dst);
        map.copy_from(data, size);
        m_graphics_device->unmap_buffer(dst);
    }

    rhi::shader_handle load_and_create_shader(tavros::core::string_view path, rhi::shader_stage stage)
    {
        const auto src = m_sl.load(path, {tavros::renderer::shader_language::glsl_460, ""});
        return m_graphics_device->create_shader({src, stage, "main"});
    }

    void save_image(tavros::assets::image_view im, tavros::core::string_view path, bool y_flip = false)
    {
        if (!im.valid()) {
            g_logger.error("Failed to save invalid image {}", path);
            return;
        }
        try {
            auto data = tavros::assets::image::encode(im, y_flip);
            m_am->open_writer(path)->write(data.data(), data.size_bytes());
        } catch (const tavros::core::file_error& e) {
            g_logger.error("Failed to save image '{}'", e.path());
        }
    }

    tavros::renderer::texture_handle bake_and_upload_font_atlas()
    {
        ZoneScopedNC("BakeFontAtlas", 0x607D8B);
        auto atlas = m_font_lib.invalidate_old_and_bake_new_atlas(96.0f, 8.0f);

        auto* cmd = m_composer->create_command_queue();
        auto  tex = m_texture_manager.load(m_stage_buffer, *cmd, atlas, "k_sdf_font_texture");
        m_composer->submit_command_queue(cmd);

        TAV_FATAL_IF(!tex, "Failed to create font atlas texture.");
        return tex;
    }

    float rand_range(float lo, float hi) noexcept
    {
        return lo + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * (hi - lo);
    }

    // ==================================================================
    // Members
    // ==================================================================

    // -- Core --
    tavros::core::shared_ptr<tavros::assets::asset_manager> m_am;
    tavros::renderer::shader_loader                         m_sl;
    tavros::core::thread_pool                               m_thread_pool;

    tavros::renderer::texture_manager m_texture_manager;

    // -- Graphics device --
    tavros::core::unique_ptr<rhi::graphics_device> m_graphics_device;
    rhi::frame_composer*                           m_composer = nullptr;
    rhi::fence_handle                              m_fence;

    // -- Offscreen RT --
    tavros::renderer::render_target m_offscreen_rt;

    // -- Pipelines --
    rhi::pipeline_handle m_fullscreen_quad_pipeline;
    rhi::pipeline_handle m_mesh_rendering_pipeline;
    rhi::pipeline_handle m_world_grid_pipeline;
    rhi::pipeline_handle m_sdf_font_pipeline;
    rhi::pipeline_handle m_particle_pipeline;
    rhi::pipeline_handle m_skybox_pipeline;
    rhi::pipeline_handle m_line2d_pipeline;
    rhi::pipeline_handle m_circle2d_pipeline;
    rhi::pipeline_handle m_sprite3d_pipeline;

    // -- Render passes --
    rhi::render_pass_handle m_main_pass;

    // -- GPU resources --
    tavros::renderer::gpu_stage_buffer m_stage_buffer;

    tavros::renderer::texture_handle m_font_texture;
    rhi::sampler_handle              m_sampler;
    rhi::sampler_handle              m_skybox_sampler;
    rhi::buffer_handle               m_stage_upload_buffer;
    rhi::buffer_handle               m_mesh_vertices_buffer;
    rhi::buffer_handle               m_mesh_indices_buffer;
    rhi::buffer_handle               m_uniform_buffer;


    tavros::renderer::texture_handle m_mesh_texture;
    tavros::renderer::texture_handle m_boom_texture;

    tavros::renderer::texture_handle m_skybox_texture;

    tavros::renderer::gpu_stream_buffer m_stream_draw;

    // -- Mesh --
    gpu_mesh_slice m_obj_mesh;

    // -- Bindings --
    rhi::buffer_binding m_scene_binding;
    size_t              m_stage_to_scene_offset = 0;

    // -- Scene --
    tavros::sandbox::free_camera     m_free_cam;
    app::scene_data                  m_scene_data;
    tavros::renderer::debug_renderer m_drenderer;

    // -- Text / fonts --
    tavros::renderer::font_library m_font_lib;
    app::text_archetype            m_fps_text;

    // -- Particles --
    tavros::particles::particle_archetype m_particles;
    tavros::particles::emitter_archetype  m_particle_emitters;
    tavros::particles::effector_archetype m_particle_effectors;

    // -- Input --
    tavros::input::input_manager m_input_manager;

    // -- Frame state --
    tavros::core::timer m_timer;
    size_t              m_frame_number = 0;
    fps_meter           m_fps_meter;
    float               m_delta_time = 0.0f;
    float               m_time = 0.0f;
    uint32              m_current_buffer_output = 0;
    bool                m_capture_next_frame = false;
};

// -------------------------------------------------------------------------
// Entry point
// -------------------------------------------------------------------------

int main()
{
    tavros::core::logger::add_consumer(
        [](auto, auto, auto msg) { std::printf("%s\n", msg.data()); }
    );

    using namespace tavros::core;

    unique_ptr<config> cfg = make_unique<config>();

;

    auto* settings = cfg->make_object("settings");
    auto* version = cfg->make_integer("version", 3);
    auto* fullscreen = cfg->make_bool("fullscreen", false);
    auto* resolution = cfg->make_object("resolution");
    auto* resolution_w = cfg->make_integer("width", 1920);
    auto* resolution_h = cfg->make_integer("height", 1080);
    auto* resolution_h2 = cfg->make_integer("height", 10802);
    auto* scale = cfg->make_float("scale", 1.5);

    settings->add_child(version);
    settings->add_child(fullscreen);
    settings->add_child(resolution);
    settings->add_child(scale);
    resolution->add_child(resolution_w);
    resolution->add_child(resolution_h);

    settings->add_last_child("key");
    settings->add_next("key", 123);
    settings->add_prev("key", 456)


    auto* node = settings->at_path("resolution.height");
    TAV_ASSERT(node != nullptr);

    TAV_ASSERT(node->get<int>() == 1080);

    return 0;


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
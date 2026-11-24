#include <tavros/renderer/passes/debug_render_pass.hpp>

#include <tavros/renderer/render_system.hpp>
#include <tavros/core/logger/logger.hpp>

#include <algorithm>

namespace
{
    tavros::core::logger logger("debug_render_pass");

    constexpr uint32 k_debug_vertex_buffer_binding_index = 1;

    constexpr uint32 k_invalid_material = 0xFFFFFFFF;

    // Index = depth mode * 3 + topology (triangles, lines, points).
    // Depth mode order matches debug_depth_test_mode: none, depth_test, depth_write.
    constexpr const char* k_material_names[] = {
        "debug_materials.triangles_no_test",
        "debug_materials.lines_no_test",
        "debug_materials.points_no_test",

        "debug_materials.triangles_test",
        "debug_materials.lines_test",
        "debug_materials.points_test",

        "debug_materials.triangles_test_write",
        "debug_materials.lines_test_write",
        "debug_materials.points_test_write",
    };

    // 1 vertex - point, 2 - line, 3 - triangle. Returns 0xFFFFFFFF for an invalid count.
    uint32 topology_index(uint32 vertex_count) noexcept
    {
        switch (vertex_count) {
        case 3:
            return 0; // triangles
        case 2:
            return 1; // lines
        case 1:
            return 2; // points
        default:
            return 0xFFFFFFFF;
        }
    }

    uint32 depth_mode_index(tavros::renderer::debug_depth_test_mode m) noexcept
    {
        const auto v = static_cast<uint32>(m);
        return v < 3 ? v : 1; // unknown value - fallback to depth_test
    }

    uint32 material_index(const tavros::renderer::debug_draw_primitive_t& p) noexcept
    {
        return depth_mode_index(p.test) * 3 + topology_index(p.verts.size);
    }

    // Draw order: depth_write -> depth_test -> none (overlay last);
    // inside one mode: triangles -> lines -> points.
    uint32 sort_key(const tavros::renderer::debug_draw_primitive_t& p) noexcept
    {
        return (2 - depth_mode_index(p.test)) * 3 + topology_index(p.verts.size);
    }
} // namespace

namespace tavros::renderer
{

    debug_render_pass::debug_render_pass(render_system* rs)
        : m_rs(rs)
    {
        auto* rm = m_rs->resource_manager();

        material::vertex_attribute vert_attribs[] = {
            {"a_pos", sizeof(debug_vertex3d_t), offsetof(debug_vertex3d_t, pos), 0},
            {"a_norm", sizeof(debug_vertex3d_t), offsetof(debug_vertex3d_t, normal), 0},
            {"a_color", sizeof(debug_vertex3d_t), offsetof(debug_vertex3d_t, color), 0},
            {"a_uv", sizeof(debug_vertex3d_t), offsetof(debug_vertex3d_t, uv), 0},
        };

        // No vertex attributes: vertices are read from an SSBO by gl_VertexID.
        rm->set_material_load_params(
            vert_attribs,
            1,
            rhi::pixel_format::depth32f /* Paste something here: depth format of the target this pass draws into.
                                       For UI it is none, but depth test needs the real format (e.g. D32F) */
        );

        for (uint32 i = 0; i < k_material_count; ++i) {
            m_materials[i] = rm->load_material(k_material_names[i]);
        }

        // Reset to default for other passes (UI, etc.)
        rm->set_material_load_params({}, 1, rhi::pixel_format::depth32f);

        m_white_tex = rm->default_texture(default_texture_type::white);

        logger.debug("debug_render_pass created");
    }

    debug_render_pass::~debug_render_pass() noexcept
    {
        auto* rm = m_rs->resource_manager();
        for (auto& m : m_materials) {
            rm->release_material(m);
        }

        logger.debug("debug_render_pass destroyed");
    }

    void debug_render_pass::upload(frame_context& ctx)
    {
        m_vertices.clear();
        m_order.clear();
        m_draws.clear();

        const auto* dd = ctx.frame_data().try_get_data<fd_debug_draw_data>();
        if (!dd) {
            return;
        }

        const auto& data = dd->data;

        const auto  prim_span = data.get<debug_draw_primitive_t>();
        const auto  vert_span = data.get<debug_vertex3d_t>();
        const auto* prims = prim_span.data();
        const auto* verts = vert_span.data();

        if (prim_span.size() == 0) {
            return;
        }

        // 1. Valid primitives only
        m_order.reserve(prim_span.size());
        for (uint32 i = 0; i < prim_span.size(); ++i) {
            if (topology_index(prims[i].verts.size) == 0xFFFFFFFF) {
                logger.warning("debug primitive with {} vertices skipped", prims[i].verts.size);
                continue;
            }
            m_order.push_back(i);
        }

        if (m_order.empty()) {
            return;
        }

        // 2. Stable sort: submission order is kept inside one group
        std::stable_sort(
            m_order.begin(), m_order.end(),
            [prims](uint32 a, uint32 b) { return sort_key(prims[a]) < sort_key(prims[b]); }
        );

        // 3. Flat vertex array in draw order + draw calls
        m_vertices.reserve(vert_span.size());

        for (const uint32 idx : m_order) {
            const auto& p = prims[idx];

            const uint32 mat = material_index(p);
            const auto   tex = p.tex.valid() ? p.tex : m_white_tex;

            if (m_draws.empty() || m_draws.back().material != mat || m_draws.back().tex != tex) {
                m_draws.push_back(draw_call_t{static_cast<uint32>(m_vertices.size()), 0, mat, tex});
            }

            for (uint32 k = 0; k < p.verts.size; ++k) {
                m_vertices.push_back(verts[p.verts.first + k]);
            }
            m_draws.back().count += p.verts.size;
        }

        // 4. Upload to GPU
        // TODO: replace with the universal ctx mechanism for uploading vertex data
        auto& vsb = ctx.vertex_stream_buffer();

        const auto bytes = sizeof(debug_vertex3d_t) * m_vertices.size();
        const auto slice = vsb.slice<uint8>(bytes);
        slice.data().copy_from(m_vertices.data(), bytes);

        for (uint32 i = 0; i < 4; ++i) {
            m_bind_buffers[i] = rhi::bind_buffer_info{slice.gpu_buffer(), static_cast<uint32>(slice.offset_bytes())};
        }
    }

    void debug_render_pass::execute(frame_context& ctx)
    {
        if (m_draws.empty()) {
            return;
        }

        auto& cmds = ctx.commands();
        cmds.push_debug_group("DebugDraw");

        auto*      rm = m_rs->resource_manager();
        const auto smp = rm->sampler(sampler_preset::linear_repeat);

        uint32              cur_material = k_invalid_material;
        rhi::texture_handle cur_tex{};

        for (const auto& d : m_draws) {
            // Pipeline, buffers and textures are re-bound after a pipeline change
            // (not sure the RHI keeps bindings between pipelines)
            const bool material_changed = d.material != cur_material;

            if (material_changed) {
                cmds.bind_pipeline(m_materials[d.material]->gpu_pipeline());
                cmds.bind_vertex_buffers(m_bind_buffers);
                cur_material = d.material;
            }

            if (material_changed || d.tex != cur_tex) {
                const rhi::texture_binding tb[] = {
                    rhi::texture_binding{d.tex, smp, 0},
                };
                cmds.bind_shader_textures(tb);
                cur_tex = d.tex;
            }

            // gl_VertexID in the shader already includes first_vertex
            cmds.draw(d.count, d.first, 1, 0);
        }

        // Cleanup after rendering
        m_vertices.clear();
        m_order.clear();
        m_draws.clear();

        cmds.pop_debug_group();
    }

} // namespace tavros::renderer

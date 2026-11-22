#include <tavros/renderer/passes/ui_render_pass.hpp>

#include <tavros/core/math.hpp>
#include <tavros/renderer/render_system.hpp>
#include <tavros/core/logger/logger.hpp>

namespace tavros::renderer
{

    struct ui_render_pass::draw_command_t
    {
        struct pattern_t
        {
            uint32     flags = 0;
            uint32     cos_sin = 0; // packed cos/sin
            math::vec2 arg0;
        };

        struct brush_t
        {
            uint32     flags = 0; // Type
            uint32     first = 0; // first color index
            uint32     num = 0;   // number of colors
            uint32     color = 0; // Only if num == 1 (solid brush)
            math::vec2 arg0;      // start point (linear) / center (radial)
            math::vec2 arg1;      // end point (linear) / arg1.x - radius (radial)
        };

        struct shape_t
        {
            uint32     flags = 0;
            uint32     range_bias = 0; //
            math::vec2 center;
            math::vec2 half_x;         // x axis half size
            math::vec2 half_y;         // y axis half size
            math::vec2 arg0;           // LINE: x - thickness; CIRCLE: x - x_axis radius, y - y_axis radius; RECT: x - radius tl, y - radius tr
            math::vec2 arg1;           // LINE: x - dash, y - gap; CIRCLE: NONE; RECT: x - radius br, y - radius bl
        };

        struct sprite_t // 48 bytes
        {
            uint32     flags = 0;
            int32      z_index = 0; // z index for the entrie batch, not per sprite
            math::vec2 center;
            math::vec2 half_x;      // x axis half size
            math::vec2 half_y;      // y axis half size
            math::vec2 src_xy;
            math::vec2 src_wh;
        };

        struct scissor_t
        {
            float left = 0.0f;
            float top = 0.0f;
            float right = 0.0f;
            float bottom = 0.0f;
        };

        brush_t   brush;
        pattern_t pattern;
        shape_t   draw_shape;
        shape_t   cut_shape;
        sprite_t  sprite; // z index stored here
        scissor_t scissor;
    };

    struct ui_render_pass::ui_batch_textures
    {
        rhi::texture_handle sprite;
        rhi::texture_handle draw_sdf;
        rhi::texture_handle cut_sdf;
    };

    struct ui_render_pass::ui_batch
    {
        uint32            first = 0;
        uint32            count = 0;
        ui_batch_textures tex;
    };

    struct ui_render_pass::ui_cmd_entry
    {
        draw_command_t    cmd;
        int32             z = 0;
        ui_batch_textures tex;
    };
} // namespace tavros::renderer

namespace
{
    namespace math = tavros::math;

    tavros::core::logger logger("ui_render_pass");

    constexpr uint32 k_gradient_repeating_shift = 4;
    constexpr uint32 k_has_sprite_flag = 0x01;
    constexpr uint32 k_sdf_max_pad_size = 0xFFFF;
    constexpr uint32 k_sdf_shift = 16;
    constexpr float  k_sdf_shape_size_divisor = 64.0f;
    constexpr uint32 k_scissor_flag = 0x100;
    constexpr uint32 k_ui_draw_buffer_binding_index = 2;
    constexpr uint32 k_ui_paint_buffer_binding_index = 1;


    using data_t = tavros::renderer::ui_render_data_t;
    using cmd_t = tavros::renderer::ui_render_pass::draw_command_t;
    using batch_tex_t = tavros::renderer::ui_render_pass::ui_batch_textures;

    enum class gpu_shape_type : uint32
    {
        none = 0,
        fill = 1,
        line = 2,
        lame_ellipse = 3,
        round_rect = 4,
        sdf = 5,
    };

    enum class gpu_brush_type : uint32
    {
        white = 0,
        linear = 1,
        radial = 2,
        conic = 3,
    };


    constexpr uint32 u(gpu_shape_type t) noexcept
    {
        return static_cast<uint32>(t);
    }
    constexpr uint32 u(gpu_brush_type t) noexcept
    {
        return static_cast<uint32>(t);
    }

    uint32 sdf_pad_bits(float pad_size) noexcept
    {
        const auto sdf_size = static_cast<uint32>(math::ceil(math::abs(pad_size * k_sdf_shape_size_divisor)));
        TAV_ASSERT(sdf_size < k_sdf_max_pad_size);
        return sdf_size << k_sdf_shift;
    }

    void build_brush(const data_t& d, tavros::core::item_view<tavros::renderer::paint_brush_t> view, cmd_t::brush_t& out)
    {
        if (!view.valid()) {
            return; // flags == white
        }

        const auto& b = d[view];

        switch (b.type) {
        case tavros::renderer::brush_fill_type::solid:
            //  ак в старом fill_color: тип linear, num == 1, цвет в brush.color
            out.flags = u(gpu_brush_type::linear);
            out.first = 0;
            out.num = 1;
            out.color = b.color.packed;
            break;

        case tavros::renderer::brush_fill_type::linear: {
            const auto& g = d[b.linear_view];
            out.flags = u(gpu_brush_type::linear) | (static_cast<uint32>(g.repeat) << k_gradient_repeating_shift);
            out.first = g.paint.first;
            out.num = g.paint.size;
            out.color = 0;
            out.arg0 = g.from;
            out.arg1 = g.to;
            break;
        }

        case tavros::renderer::brush_fill_type::radial: {
            const auto& g = d[b.radial_view];
            out.flags = u(gpu_brush_type::radial) | (static_cast<uint32>(g.repeat) << k_gradient_repeating_shift);
            out.first = g.paint.first;
            out.num = g.paint.size;
            out.color = 0;
            out.arg0 = g.center;
            out.arg1 = math::vec2(g.radius, 0.0f);
            break;
        }

        case tavros::renderer::brush_fill_type::conic: {
            const auto& g = d[b.conic_view];

            float sweep_rad = g.sweep.radians();
            if (sweep_rad > math::k_two_pi<float>) {
                sweep_rad = std::fmod(sweep_rad, math::k_two_pi<float>);
            }
            if (sweep_rad == 0.0f) {
                sweep_rad = math::k_two_pi<float>;
            }

            out.flags = u(gpu_brush_type::conic) | (static_cast<uint32>(g.repeat) << k_gradient_repeating_shift);
            out.first = g.paint.first;
            out.num = g.paint.size;
            out.color = 0;
            out.arg0 = g.center;
            out.arg1 = math::vec2(g.start.radians(), sweep_rad);
            break;
        }
        }
    }

    void build_pattern(const data_t& d, tavros::core::item_view<tavros::renderer::fill_pattern_t> view, cmd_t::pattern_t& out)
    {
        if (!view.valid()) {
            return; // flags == 0
        }

        const auto& p = d[view];
        const auto  c = math::cos(p.angle.radians());
        const auto  s = math::sin(p.angle.radians());

        out.flags = static_cast<uint32>(p.type); // см. примечание про нумерацию в конце
        out.cos_sin = math::pack_snorm2x16(c, s);
        out.arg0 = math::vec2(p.size0, p.size1);
    }

    // tex заполн€етс€ только дл€ sdf-фигур
    void build_shape(const data_t& d, tavros::core::item_view<tavros::renderer::draw_shape_t> view, cmd_t::shape_t& out, tavros::renderer::rhi::texture_handle& sdf_tex)
    {
        if (!view.valid()) {
            return; // flags == none
        }

        const auto& s = d[view];

        out.range_bias = math::pack_half2x16(s.shadow_radius, s.shadow_bias);

        switch (s.type) {
        case tavros::renderer::shape_type::quad: {
            const auto& q = d[s.quad_view];
            out.flags = u(gpu_shape_type::fill);
            out.center = q.center;
            out.half_x = q.half_x;
            out.half_y = q.half_y;
            break;
        }

        case tavros::renderer::shape_type::line: {
            const auto&      l = d[s.line_view];
            const math::vec2 half_dir = (l.to - l.from) * 0.5f;
            const float      len = math::length(half_dir);
            const float      inv_len = len > 0.0f ? 1.0f / len : 0.0f;

            out.flags = u(gpu_shape_type::line);
            out.center = (l.from + l.to) * 0.5f;
            out.half_x = half_dir;
            out.half_y = math::vec2(-half_dir.y * inv_len, half_dir.x * inv_len) * (l.thickness * 0.5f);
            out.arg0.x = l.thickness;
            out.arg1.x = l.dash;
            out.arg1.y = l.gap;
            break;
        }

        case tavros::renderer::shape_type::superellipse: {
            const auto& e = d[s.superellipse_view];
            out.flags = u(gpu_shape_type::lame_ellipse);
            out.center = e.center;
            out.half_x = e.half_x;
            out.half_y = e.half_y;
            out.arg0 = math::vec2(e.squash, 0.0f);
            out.arg1 = math::vec2(0.0f, 0.0f);
            break;
        }

        case tavros::renderer::shape_type::rect: {
            const auto& r = d[s.rect_view];
            out.flags = u(gpu_shape_type::round_rect);
            out.center = r.center;
            out.half_x = r.half_x;
            out.half_y = r.half_y;
            out.arg0 = math::vec2(r.radius.top_left, r.radius.top_right);
            out.arg1 = math::vec2(r.radius.bottom_right, r.radius.bottom_left);
            break;
        }

        case tavros::renderer::shape_type::sdf: {
            const auto& f = d[s.sdf_view];
            out.flags = u(gpu_shape_type::sdf) | sdf_pad_bits(f.sdf_pad_size);
            out.center = f.center;
            out.half_x = f.half_x;
            out.half_y = f.half_y;
            out.arg0 = math::vec2(f.atlas_rect.x, f.atlas_rect.y);
            out.arg1 = math::vec2(f.atlas_rect.z, f.atlas_rect.w);
            sdf_tex = f.tex;
            break;
        }
        }
    }

    // ------------------------------ Command ------------------------------

    tavros::renderer::ui_render_pass::ui_cmd_entry build_entry(
        const data_t& d, const tavros::renderer::ui_draw_command_t& cmd,
        tavros::renderer::rhi::texture_handle white_tex, tavros::renderer::rhi::texture_handle debug_tex
    )
    {
        tavros::renderer::ui_render_pass::ui_cmd_entry e;
        e.z = cmd.z_order;
        e.tex = {debug_tex, white_tex, white_tex};

        build_brush(d, cmd.brush, e.cmd.brush);
        build_pattern(d, cmd.pattern, e.cmd.pattern);
        build_shape(d, cmd.draw_shape, e.cmd.draw_shape, e.tex.draw_sdf);
        build_shape(d, cmd.cut_shape, e.cmd.cut_shape, e.tex.cut_sdf);

        e.cmd.sprite.z_index = cmd.z_order;

        if (cmd.sprite.valid()) {
            const auto& sp = d[cmd.sprite];
            e.cmd.sprite.flags = k_has_sprite_flag;
            e.cmd.sprite.center = sp.center;
            e.cmd.sprite.half_x = sp.half_x;
            e.cmd.sprite.half_y = sp.half_y;
            e.cmd.sprite.src_xy = math::vec2(sp.atlas_rect.x, sp.atlas_rect.y);
            e.cmd.sprite.src_wh = math::vec2(sp.atlas_rect.z, sp.atlas_rect.w);
            e.tex.sprite = sp.tex;
        }

        if (cmd.scissor.valid()) {
            const auto& sc = d[cmd.scissor];
            e.cmd.scissor = {sc.left, sc.top, sc.right, sc.bottom};
            e.cmd.sprite.flags |= k_scissor_flag;
        }

        return e;
    }

    bool same_textures(const batch_tex_t& a, const batch_tex_t& b) noexcept
    {
        return a.sprite == b.sprite && a.draw_sdf == b.draw_sdf && a.cut_sdf == b.cut_sdf;
    }
} // namespace

namespace tavros::renderer
{

    ui_render_pass::ui_render_pass(render_system* rs)
        : m_rs(rs)
    {
        m_rs->resource_manager()->set_material_load_params({}, 1, rhi::pixel_format::none);
        m_ui_mt = m_rs->resource_manager()->load_material("mt.ui_material");

        m_debug_tex = m_rs->resource_manager()->default_texture(default_texture_type::debug);
        m_white_tex = m_rs->resource_manager()->default_texture(default_texture_type::white);

        logger.debug("ui_render_pass created");
    }

    ui_render_pass::~ui_render_pass() noexcept
    {
        m_rs->resource_manager()->release_material(m_ui_mt);

        logger.debug("ui_render_pass destroyed");
    }

    void ui_render_pass::upload(frame_context& ctx)
    {
        m_entries.clear();
        m_draw_commands.clear();
        m_batches.clear();

        const auto* ui_rd = ctx.frame_data().try_get_data<fd_ui_draw_data>();
        if (!ui_rd) {
            return;
        }

        const auto& data = ui_rd->data;

        // Decode
        const auto commands = data.get<ui_draw_command_t>();
        m_entries.reserve(commands.size());
        for (const auto& cmd : commands) {
            m_entries.push_back(build_entry(data, cmd, m_white_tex, m_debug_tex));
        }

        if (m_entries.empty()) {
            return;
        }

        // Sort UI commands by z-order (stable sort to maintain order of commands with same z)
        std::stable_sort(
            m_entries.begin(), m_entries.end(),
            [](const ui_cmd_entry& a, const ui_cmd_entry& b) { return a.z < b.z; }
        );

        // Array of draw commands and batches
        m_draw_commands.reserve(m_entries.size());
        for (uint32 i = 0; i < m_entries.size(); ++i) {
            const auto& e = m_entries[i];

            if (m_batches.empty() || !same_textures(m_batches.back().tex, e.tex)) {
                m_batches.push_back(ui_batch{i, 0, e.tex});
            }

            m_batches.back().count++;
            m_draw_commands.push_back(e.cmd);
        }

        // Upload to GPU
        auto& sb = ctx.stream_buffer();

        const auto cmd_bytes = sizeof(draw_command_t) * m_draw_commands.size();
        const auto cmd_slice = sb.slice<uint8>(cmd_bytes);
        cmd_slice.data().copy_from(m_draw_commands.data(), cmd_bytes);

        m_draw_buffer_view = rhi::buffer_binding{
            cmd_slice.gpu_buffer(),
            static_cast<uint32>(cmd_slice.offset_bytes()),
            static_cast<uint32>(cmd_slice.size_bytes()),
            k_ui_draw_buffer_binding_index
        };

        const auto stops = data.get<color_stop_t>();
        const auto stops_bytes = sizeof(color_stop_t) * stops.size();
        const auto stops_slice = sb.slice<uint8>(stops_bytes);
        stops_slice.data().copy_from(stops.data(), stops_bytes);

        m_paint_buffer_view = rhi::buffer_binding{
            stops_slice.gpu_buffer(),
            static_cast<uint32>(stops_slice.offset_bytes()),
            static_cast<uint32>(stops_slice.size_bytes()),
            k_ui_paint_buffer_binding_index
        };
    }

    void ui_render_pass::execute(frame_context& ctx)
    {
        if (m_batches.empty()) {
            return;
        }

        ctx.commands().push_debug_group("DrawUI");

        auto& cmds = ctx.commands();

        cmds.bind_pipeline(m_ui_mt->gpu_pipeline());

        const rhi::buffer_binding sb[] = {m_paint_buffer_view, m_draw_buffer_view};
        cmds.bind_shader_buffers(sb);

        auto*      rm = m_rs->resource_manager();
        const auto tex0_default = rm->default_texture(default_texture_type::debug);
        const auto tex1_default = rm->default_texture(default_texture_type::white);
        const auto tex2_default = rm->default_texture(default_texture_type::white);
        const auto smp = rm->sampler(sampler_preset::linear_repeat);

        for (const auto& b : m_batches) {
            const auto tex0 = b.tex.sprite.valid() ? b.tex.sprite : tex0_default;
            const auto tex1 = b.tex.draw_sdf.valid() ? b.tex.draw_sdf : tex1_default;
            const auto tex2 = b.tex.cut_sdf.valid() ? b.tex.cut_sdf : tex2_default;

            const rhi::texture_binding tb[] = {
                rhi::texture_binding{tex0, smp, 0},
                rhi::texture_binding{tex1, smp, 1},
                rhi::texture_binding{tex2, smp, 2},
            };
            cmds.bind_shader_textures(tb);
            cmds.draw(6, 0, b.count, b.first);
        }

        // Cleanup after rendering
        m_entries.clear();
        m_draw_commands.clear();
        m_batches.clear();


        ctx.commands().pop_debug_group();
    }

} // namespace tavros::renderer

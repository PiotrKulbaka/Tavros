#include <tavros/renderer/passes/ui_render_pass.hpp>

#include <tavros/core/math.hpp>
#include <tavros/renderer/render_system.hpp>
#include <tavros/core/logger/logger.hpp>

namespace
{
    namespace math = tavros::math;

    tavros::core::logger logger("ui_render_pass");

    struct draw_command_t
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

    const auto sdf_size = static_cast<uint32>(math::ceil(math::abs(sdf_pad_size * k_sdf_shape_size_divisor)));
    //TAV_ASSERT(sdf_size < k_sdf_max_pad_size);


    constexpr int32  k_solid_brush_type_id = 0;
    constexpr int32  k_linear_gradient_brush_type_id = 1;
    constexpr int32  k_radial_gradient_brush_type_id = 2;
    constexpr uint32 k_gradient_repeating_shift = 4;
    constexpr uint32 k_has_sprite_flag = 0x01;
    constexpr uint32 k_sdf_max_pad_size = 0xFFFF;
    constexpr uint32 k_sdf_shift = 16;
    constexpr float  k_sdf_shape_size_divisor = 64.0f;
    constexpr uint32 k_scissor_flag = 0x100;
} // namespace

namespace tavros::renderer
{

    ui_render_pass::ui_render_pass(render_system* rs)
        : m_rs(rs)
    {
        m_rs->resource_manager()->set_material_load_params({}, 1, rhi::pixel_format::none);
        m_ui_mt = m_rs->resource_manager()->load_material("mt.uir");

        logger.debug("ui_render_pass created");
    }

    ui_render_pass::~ui_render_pass() noexcept
    {
        m_rs->resource_manager()->release_material(m_ui_mt);

        logger.debug("ui_render_pass destroyed");
    }

    void ui_render_pass::upload(frame_context& ctx)
    {
        const ui_render_data_t& data = ctx.frame_data().get_data<fdt_ui_draw_data>().data;
        
        /*


            auto& sb = ctx.stream_buffer();

        const auto draw_buffer_size_bytes = sizeof(ui_draw_builder::draw_command_t) * m_draw_commands.size();
        const auto draw_buffer_slice = sb.slice<uint8>(draw_buffer_size_bytes);
        draw_buffer_slice.data().copy_from(m_draw_commands.data(), draw_buffer_size_bytes);

        m_draw_buffer_view = rhi::buffer_binding{
            draw_buffer_slice.gpu_buffer(),
            static_cast<uint32>(draw_buffer_slice.offset_bytes()),
            static_cast<uint32>(draw_buffer_slice.size_bytes()),
            k_ui_draw_buffer_binding_index
        };

        m_paint_buffer_view = m_paint_collection.upload(sb);*/
    }

    void ui_render_pass::execute(frame_context& ctx)
    {
        //if (m_draw_textures.empty()) {
        //    return;
        //}

        //auto& cmds = ctx.commands();

        //cmds.bind_pipeline(m_ui_mt->gpu_pipeline());

        //rhi::buffer_binding sb[] = {m_draw_buffer_view, m_paint_buffer_view};
        //cmds.bind_shader_buffers(sb);

        //auto*      rm = m_rs->resource_manager();
        //const auto tex0_default = rm->default_texture(default_texture_type::debug);
        //const auto tex1_default = rm->default_texture(default_texture_type::white);
        //const auto tex2_default = rm->default_texture(default_texture_type::white);
        //const auto smp = rm->sampler(sampler_preset::linear_repeat);


        ///*for (size_t i = 0; i < m_draw_textures.size(); ++i) {
        //    const auto& textures = m_draw_textures[i];
        //    const auto tex0 = textures.texture ? textures.texture : tex0_default;
        //    const auto tex1 = textures.draw_sdf ? textures.draw_sdf : tex1_default;
        //    const auto tex2 = textures.cut_sdf ? textures.cut_sdf : tex2_default;

        //    const rhi::texture_binding tb[] = {
        //        rhi::texture_binding{tex0, smp, 0},
        //        rhi::texture_binding{tex1, smp, 1},
        //        rhi::texture_binding{tex2, smp, 2},
        //    };
        //    m_cmd->bind_shader_textures(tb);
        //    m_cmd->draw(6, 0, 1, static_cast<uint32>(i));
        //}*/


        //// Текущий "разрешённый" набор текстур батча. rhi::texture_handle{} == 0 == "ещё не зафиксирован".
        //rhi::texture_handle batch_tex0{};
        //rhi::texture_handle batch_tex1{};
        //rhi::texture_handle batch_tex2{};
        //size_t              batch_start = 0;

        //// Биндит текстуры текущего батча (с фолбэком на дефолтные, если слот так и не был зафиксирован)
        //// и рисует инстансингом диапазон [batch_start, end).
        //auto flush_batch = [&](size_t end) {
        //    const auto count = static_cast<uint32>(end - batch_start);
        //    const auto tex0 = batch_tex0 ? batch_tex0 : tex0_default;
        //    const auto tex1 = batch_tex1 ? batch_tex1 : tex1_default;
        //    const auto tex2 = batch_tex2 ? batch_tex2 : tex2_default;

        //    const rhi::texture_binding tb[] = {
        //        rhi::texture_binding{tex0, smp, 0},
        //        rhi::texture_binding{tex1, smp, 1},
        //        rhi::texture_binding{tex2, smp, 2},
        //    };
        //    cmds.bind_shader_textures(tb);
        //    cmds.draw(6, 0, count, static_cast<uint32>(batch_start));
        //};

        //for (size_t i = 0; i < m_draw_textures.size(); ++i) {
        //    const auto& textures = m_draw_textures[i];

        //    // Разрыв батча: слот уже зафиксирован (не 0) в текущем батче,
        //    // а у текущей команды в этом же слоте другая ненулевая текстура.
        //    const bool breaks =
        //        (batch_tex0.valid() && textures.texture.valid() && textures.texture != batch_tex0) || (batch_tex1.valid() && textures.draw_sdf.valid() && textures.draw_sdf != batch_tex1) || (batch_tex2.valid() && textures.cut_sdf.valid() && textures.cut_sdf != batch_tex2);

        //    if (breaks) {
        //        flush_batch(i);
        //        batch_start = i;
        //        batch_tex0 = {};
        //        batch_tex1 = {};
        //        batch_tex2 = {};
        //    }

        //    // Фиксируем слот, если он ещё не зафиксирован в этом батче, а у команды он не 0.
        //    // Если у команды 0 - слот просто наследует то, что уже есть в батче (или остаётся незафиксированным).
        //    if (!batch_tex0.valid() && textures.texture.valid()) {
        //        batch_tex0 = textures.texture;
        //    }
        //    if (!batch_tex1.valid() && textures.draw_sdf.valid()) {
        //        batch_tex1 = textures.draw_sdf;
        //    }
        //    if (!batch_tex2.valid() && textures.cut_sdf.valid()) {
        //        batch_tex2 = textures.cut_sdf;
        //    }
        //}

        //flush_batch(m_draw_textures.size());


        //// Clean buffers
        //m_scissor_stack.clear();
        //m_draw_commands.clear();
        //m_draw_textures.clear();
        //m_paint_collection.clear();
    }

} // namespace tavros::renderer

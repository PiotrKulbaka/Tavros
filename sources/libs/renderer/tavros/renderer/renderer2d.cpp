#include <tavros/renderer/renderer2d.hpp>

#include <tavros/renderer/components/atlas_rect.hpp>
#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("renderer2d");

    namespace math = tavros::math;

    constexpr int32 k_solid_brush_type_id = 0;
    constexpr int32 k_linear_gradient_brush_type_id = 1;
    constexpr int32 k_radial_gradient_brush_type_id = 2;

    struct glyph_instance_data
    {
        float                          transform[3][2];
        tavros::renderer::atlas_rect_t rect;
        tavros::math::color32          fill_color;
        tavros::math::color32          outline_color;
    };

    enum class shape_type : uint8
    {
        none = 0,
        fill = 1,
        line = 2,
        lame_ellipse = 3,
        round_rect = 4,
    };

    enum class brush_type : uint8
    {
        white = 0,
        linear = 1,
        radial = 2,
		conic = 3,
    };
    
	constexpr uint32 k_gradient_repeating_shift = 4;
	constexpr uint32 k_has_sprite_flag = 0x01;

} // namespace

namespace tavros::renderer
{

    draw_builder::draw_builder(renderer2d* r) noexcept
        : m_r(r)
    {
    }

    draw_builder& draw_builder::fill_color(math::color32 color) noexcept
    {
		m_pc.brush.flags = static_cast<uint32>(brush_type::linear);
		m_pc.brush.first = 0;
		m_pc.brush.num = 1;
		m_pc.brush.color = color.packed;
		m_pc.brush.arg0 = math::vec2(0.0f, 0.0f);
		m_pc.brush.arg1 = math::vec2(0.0f, 0.0f);

        return *this;
    }

    draw_builder& draw_builder::fill_linear(core::buffer_view<color_stop> gradient, math::vec2 from, math::vec2 to, gradient_repeat repeat) noexcept
    {
        if (gradient.size() == 0) {
            return *this;
        }
        
        if (gradient.size() == 1) {
            return fill_color(gradient[0].color);
        }

        auto v = m_r->m_paint_collection.add_paint(gradient);
        m_pc.brush.flags = static_cast<uint32>(brush_type::linear) | (static_cast<uint32>(repeat) << k_gradient_repeating_shift);
		m_pc.brush.first = v.first;
		m_pc.brush.num = v.number;
		m_pc.brush.color = 0;
		m_pc.brush.arg0 = from;
		m_pc.brush.arg1 = to;

        return *this;
    }

    draw_builder& draw_builder::fill_radial(core::buffer_view<color_stop> gradient, math::vec2 center, float radius, gradient_repeat repeat) noexcept
    {
        if (gradient.size() == 0) {
            return *this;
        }
        
        if (gradient.size() == 1) {
            return fill_color(gradient[0].color);
        }

        auto v = m_r->m_paint_collection.add_paint(gradient);
        m_pc.brush.flags = static_cast<uint32>(brush_type::radial) | (static_cast<uint32>(repeat) << k_gradient_repeating_shift);
		m_pc.brush.first = v.first;
		m_pc.brush.num = v.number;
		m_pc.brush.color = 0;
		m_pc.brush.arg0 = center;
		m_pc.brush.arg1 = math::vec2(radius, 0.0f);

        return *this;
    }

    draw_builder& draw_builder::fill_conic(core::buffer_view<color_stop> gradient, math::vec2 center, float start_angle_rad, gradient_repeat repeat) noexcept
    {
        if (gradient.size() == 0) {
            return *this;
        }
        
        if (gradient.size() == 1) {
            return fill_color(gradient[0].color);
        }

        auto v = m_r->m_paint_collection.add_paint(gradient);
		m_pc.brush.flags = static_cast<uint32>(brush_type::conic) | (static_cast<uint32>(repeat) << k_gradient_repeating_shift);
		m_pc.brush.first = v.first;
		m_pc.brush.num = v.number;
		m_pc.brush.color = 0;
		m_pc.brush.arg0 = center;
		m_pc.brush.arg1 = math::vec2(start_angle_rad, 0.0f);

        return *this;
    }

    draw_builder& draw_builder::cut_line(math::vec2 from, math::vec2 to, float thickness, float dash, float gap)
    {
        const math::vec2 half_dir = (to - from) * 0.5f;
        const float inv_len = 1.0f / math::length(half_dir);
        m_pc.cut_shape.flags = static_cast<uint32>(shape_type::line);
        m_pc.cut_shape.reserved = 0;
		m_pc.cut_shape.center = (from + to) * 0.5f;
        m_pc.cut_shape.x_axis = half_dir;
		m_pc.cut_shape.y_axis = math::vec2(-half_dir.y * inv_len, half_dir.x * inv_len) * (thickness * 0.5f);
		m_pc.cut_shape.arg0.x = thickness;
		m_pc.cut_shape.arg1.x = dash;
		m_pc.cut_shape.arg1.y = gap;
        return *this;
    }

    draw_builder& draw_builder::cut_superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash)
    {
        m_pc.cut_shape.flags = static_cast<uint32>(shape_type::lame_ellipse);
        m_pc.cut_shape.reserved = 0;
		m_pc.cut_shape.center = center;
        m_pc.cut_shape.x_axis = half_x;
		m_pc.cut_shape.y_axis = half_y;
		m_pc.cut_shape.arg0 = math::vec2(squash, 0.0f);
		m_pc.cut_shape.arg1 = math::vec2(0.0f, 0.0f);
        return *this;
    }

    draw_builder& draw_builder::cut_rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius)
    {
        m_pc.cut_shape.flags = static_cast<uint32>(shape_type::round_rect);
        m_pc.cut_shape.reserved = 0;
		m_pc.cut_shape.center = center;
        m_pc.cut_shape.x_axis = half_x;
        m_pc.cut_shape.y_axis = half_y;
		m_pc.cut_shape.arg0 = math::vec2(radius.top_left, radius.top_right);
		m_pc.cut_shape.arg1 = math::vec2(radius.bottom_right, radius.bottom_left);
        return *this;
    }

    draw_builder& draw_builder::sprite(texture_ref texture, math::vec2 center, math::vec2 half_x, math::vec2 half_y, math::vec4 sprite_rect)
    {
        m_pc.sprite.flags = static_cast<uint32>(k_has_sprite_flag);
		m_pc.sprite.reserved = 0;
		m_pc.sprite.center = center;
		m_pc.sprite.x_axis = half_x;
        m_pc.sprite.y_axis = half_y;
		m_pc.sprite.src_rect = sprite_rect;

		m_r->m_cmd->bind_shader_textures(rhi::texture_binding{ texture->gpu_texture(), m_r->m_rm->sampler(sampler_preset::nearest_repeat), 0});
        
        return *this;
    }

    void draw_builder::line(math::vec2 from, math::vec2 to, float thickness, float dash, float gap) noexcept
    {
        const math::vec2 half_dir = (to - from) * 0.5f;
        const float inv_len = 1.0f / math::length(half_dir);
        m_pc.draw_shape.flags = static_cast<uint32>(shape_type::line);
        m_pc.draw_shape.reserved = 0;
		m_pc.draw_shape.center = (from + to) * 0.5f;
		m_pc.draw_shape.x_axis = half_dir;
		m_pc.draw_shape.y_axis = math::vec2(-half_dir.y * inv_len, half_dir.x * inv_len) * (thickness * 0.5f);
		m_pc.draw_shape.arg0.x = thickness;
		m_pc.draw_shape.arg1.x = dash;
		m_pc.draw_shape.arg1.y = gap;
    }

    void draw_builder::superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash) noexcept
    {
        m_pc.draw_shape.flags = static_cast<uint32>(shape_type::lame_ellipse);
        m_pc.draw_shape.reserved = 0;
		m_pc.draw_shape.center = center;
        m_pc.draw_shape.x_axis = half_x;
		m_pc.draw_shape.y_axis = half_y;
		m_pc.draw_shape.arg0 = math::vec2(squash, 0.0f);
		m_pc.draw_shape.arg1 = math::vec2(0.0f, 0.0f);
    }

    void draw_builder::rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius) noexcept
    {
        m_pc.draw_shape.flags = static_cast<uint32>(shape_type::round_rect);
        m_pc.draw_shape.reserved = 0;
		m_pc.draw_shape.center = center;
        m_pc.draw_shape.x_axis = half_x;
        m_pc.draw_shape.y_axis = half_y;
		m_pc.draw_shape.arg0 = math::vec2(radius.top_left, radius.top_right);
		m_pc.draw_shape.arg1 = math::vec2(radius.bottom_right, radius.bottom_left);
    }

    void draw_builder::draw()
    {
		m_r->m_cmd->bind_pipeline(m_r->m_r2d_mt->gpu_pipeline());
		m_r->m_cmd->push_constant(m_pc);
		auto b = m_r->m_paint_collection.upload(m_r->m_storage_buffer);
		m_r->m_cmd->bind_shader_buffers(b);
		m_r->m_cmd->draw(6);

		m_r->m_paint_collection.clear(); // TODO: remove this line, it is just for testing
    }

















    renderer2d::renderer2d(rhi::graphics_device* gdevice, resource_manager* rm)
        : m_gdevice(gdevice)
        , m_rm(rm)
    {
        init();
    }

    renderer2d::~renderer2d() noexcept
    {
        shutdown();
    }

    void renderer2d::begin_frame() noexcept
    {
        if (m_cmd) {
            logger.error("Previous frame is not ended.");
            return;
        }
        m_cmd = m_gdevice->create_command_queue();
        m_uniform_buffer.reset();
        m_storage_buffer.reset();
        m_vertices_buffer.reset();

        m_paint_collection.clear();
        m_brush_collection.clear();
        m_shape_collection.clear();
    }

    void renderer2d::end_frame() noexcept
    {
        if (!m_cmd) {
            logger.error("Frame is not started.");
            return;
        }
        m_gdevice->submit_command_queue(m_cmd);
        m_cmd = nullptr;
    }

    draw_builder renderer2d::line(math::vec2 from, math::vec2 to, float thickness, float dash, float gap)
    {
        draw_builder b(this);
		b.line(from, to, thickness, dash, gap);
		return b;
    }

    draw_builder renderer2d::superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash)
    {
        draw_builder b(this);
		b.superellipse(center, half_x, half_y, squash);
		return b;
    }

    draw_builder renderer2d::rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius)
    {
		draw_builder b(this);
		b.rect(center, half_x, half_y, radius);
		return b;
    }

    /*

    void renderer2d::draw_text(const text_archetype& text, math::vec2 pos, float fill_treshold, float outline_treshold)
    {
        TAV_ASSERT(m_cmd);
        if (!m_cmd) {
            logger.error("Failed to draw text: frame is not started.");
            return;
        }

        struct text_data
        {
            math::vec2 pos;
            uint8      fill_threshold;
            uint8      outline_threshold;
            uint8      use_fill_brush_mask : 1;
            uint8      use_outline_brush_mask : 1;
        };

        auto  slice = m_vertices_buffer.slice<glyph_instance_data>(text.size());
        auto* dst = slice.data().data();

        text.view<const atlas_rect_t, const rect_layout_c, const position2d_c, const glyph_style_c>()
            .each([&](const auto& r, const auto& l, const auto& p, const auto& st) {
                const auto size = l.size();

                dst->transform[0][0] = size.width;
                dst->transform[0][1] = 0.0f;
                dst->transform[1][0] = 0.0f;
                dst->transform[1][1] = size.height;
                dst->transform[2][0] = p.value.x + l.left;
                dst->transform[2][1] = p.value.y + l.top;
                dst->rect = r;
                dst->fill_color = st.fill_color;
                dst->outline_color = st.outline_color;

                ++dst;
            });

        text_data t;
        t.pos = pos;
        t.fill_threshold = 255 - static_cast<uint8>(fill_treshold * 255.0f);
        t.outline_threshold = static_cast<uint8>(outline_treshold * 255.0f);
        t.use_fill_brush_mask = m_text_use_fill_mask ? 1 : 0;
        t.use_outline_brush_mask = m_text_use_outline_mask ? 1 : 0;

        m_cmd->bind_pipeline(m_text_mt->gpu_pipeline());
        m_cmd->bind_shader_textures(rhi::texture_binding{m_rm->fonts_texture(), m_rm->sampler(m_text_smp), 0});
        m_cmd->push_constant(t);
        const rhi::bind_buffer_info bufs[] = {{slice.gpu_buffer(), slice.offset_bytes()}, {slice.gpu_buffer(), slice.offset_bytes()}};
        m_cmd->bind_vertex_buffers(bufs);
        m_cmd->draw(6, 0, static_cast<uint32>(text.size()));
    }

    void renderer2d::set_clip_rect(geometry::aabb2 aabb) noexcept
    {
        rhi::scissor_info si;
        si.left = static_cast<int32>(std::floor(aabb.min.x));
        si.top = static_cast<int32>(std::floor(aabb.min.y));
        si.width = static_cast<int32>(std::ceil(aabb.width()));
        si.height = static_cast<int32>(std::ceil(aabb.height()));
        m_cmd->set_scissor(si);
    }

    void renderer2d::reset_clip_rect() noexcept
    {
        m_cmd->reset_scissor();
    }*/

    void renderer2d::init()
    {
        // TODO: change to buffer pool
        m_uniform_buffer.init(m_gdevice, 64 * 1024 * 1024, rhi::buffer_usage::constant);
        m_storage_buffer.init(m_gdevice, 64 * 1024 * 1024, rhi::buffer_usage::storage);
        m_vertices_buffer.init(m_gdevice, 64 * 1024 * 1024, rhi::buffer_usage::vertex);
        m_rm->set_material_load_params({}, 1, rhi::pixel_format::none);
        /*m_line_mt = m_rm->load_material("mt.line2d");
        m_rect_mt = m_rm->load_material("mt.rect2d");
        m_circle_mt = m_rm->load_material("mt.circle2d");*/
       // m_sprite_mt = m_rm->load_material("mt.sprite2d");

        m_r2d_mt = m_rm->load_material("mt.r2d");

        core::fixed_vector<material::vertex_attribute, 2> vert_attribs;
        vert_attribs.push_back(material::vertex_attribute{"a_gpyph_transform", sizeof(glyph_instance_data), offsetof(glyph_instance_data, transform), 1});
        vert_attribs.push_back(material::vertex_attribute{"a_bounds_and_color", sizeof(glyph_instance_data), offsetof(glyph_instance_data, rect), 1});
        m_rm->set_material_load_params(vert_attribs, 1, rhi::pixel_format::none);
        //m_text_mt = m_rm->load_material("mt.text2d");
    }

    void renderer2d::shutdown() noexcept
    {
        m_uniform_buffer.shutdown();
        m_storage_buffer.shutdown();
        /*m_rm->release_material(m_line_mt);
        m_rm->release_material(m_rect_mt);
        m_rm->release_material(m_circle_mt);*/
        /*m_rm->release_material(m_sprite_mt);
        m_rm->release_material(m_text_mt);*/
        m_rm->release_material(m_r2d_mt);
    }
} // namespace tavros::renderer

#include <tavros/renderer/ui_renderer.hpp>

#include <tavros/renderer/components/atlas_rect.hpp>
#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("ui_renderer");

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
        sdf = 5,
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
    constexpr uint32 k_sdf_max_pad_size = 0xFFFF;
    constexpr uint32 k_sdf_shift = 16;
    constexpr float  k_sdf_shape_size_divisor = 64.0f;

} // namespace

namespace tavros::renderer
{

    ui_draw_builder::ui_draw_builder(ui_renderer* r) noexcept
        : m_r(r)
    {
        m_draw_sdf = m_r->m_rm->default_texture(default_texture_type::white);
        m_cut_sdf = m_r->m_rm->default_texture(default_texture_type::white);
        m_draw_texture = m_r->m_rm->default_texture(default_texture_type::debug);
    }

    ui_draw_builder::~ui_draw_builder() noexcept
    {
        TAV_ASSERT(m_drawn);
    }

    ui_draw_builder& ui_draw_builder::fill_color(math::color32 color) noexcept
    {
        TAV_ASSERT(!m_has_fill);
        TAV_DEBUG_ONLY(m_has_fill = true);

        m_batch.brush.flags = static_cast<uint32>(brush_type::linear);
        m_batch.brush.first = 0;
        m_batch.brush.num = 1;
        m_batch.brush.color = color.packed;
        m_batch.brush.arg0 = math::vec2(0.0f, 0.0f);
        m_batch.brush.arg1 = math::vec2(0.0f, 0.0f);

        return *this;
    }

    ui_draw_builder& ui_draw_builder::fill_linear(core::buffer_view<color_stop> gradient, math::vec2 from, math::vec2 to, gradient_repeat repeat) noexcept
    {
        TAV_ASSERT(!m_has_fill);
        TAV_DEBUG_ONLY(m_has_fill = true);

        if (gradient.size() == 0) {
            return *this;
        }

        if (gradient.size() == 1) {
            return fill_color(gradient[0].color);
        }

        auto v = m_r->m_paint_collection.add_paint(gradient);
        m_batch.brush.flags = static_cast<uint32>(brush_type::linear) | (static_cast<uint32>(repeat) << k_gradient_repeating_shift);
        m_batch.brush.first = v.first;
        m_batch.brush.num = v.number;
        m_batch.brush.color = 0;
        m_batch.brush.arg0 = from;
        m_batch.brush.arg1 = to;

        return *this;
    }

    ui_draw_builder& ui_draw_builder::fill_radial(core::buffer_view<color_stop> gradient, math::vec2 center, float radius, gradient_repeat repeat) noexcept
    {
        TAV_ASSERT(!m_has_fill);
        TAV_DEBUG_ONLY(m_has_fill = true);

        if (gradient.size() == 0) {
            return *this;
        }

        if (gradient.size() == 1) {
            return fill_color(gradient[0].color);
        }

        auto v = m_r->m_paint_collection.add_paint(gradient);
        m_batch.brush.flags = static_cast<uint32>(brush_type::radial) | (static_cast<uint32>(repeat) << k_gradient_repeating_shift);
        m_batch.brush.first = v.first;
        m_batch.brush.num = v.number;
        m_batch.brush.color = 0;
        m_batch.brush.arg0 = center;
        m_batch.brush.arg1 = math::vec2(radius, 0.0f);

        return *this;
    }

    ui_draw_builder& ui_draw_builder::fill_conic(core::buffer_view<color_stop> gradient, math::vec2 center, math::angle start, math::angle sweep, gradient_repeat repeat) noexcept
    {
        TAV_ASSERT(!m_has_fill);
        TAV_DEBUG_ONLY(m_has_fill = true);

        if (gradient.size() == 0) {
            return *this;
        }

        if (gradient.size() == 1) {
            return fill_color(gradient[0].color);
        }

        float sweep_rad = sweep.radians();
        if (sweep.radians() > math::k_two_pi<float>) {
            std::modf(sweep.radians(), &sweep_rad);
        }

        if (sweep_rad == 0.0f) {
            sweep_rad = math::k_two_pi<float>;
        }

        auto v = m_r->m_paint_collection.add_paint(gradient);
        m_batch.brush.flags = static_cast<uint32>(brush_type::conic) | (static_cast<uint32>(repeat) << k_gradient_repeating_shift);
        m_batch.brush.first = v.first;
        m_batch.brush.num = v.number;
        m_batch.brush.color = 0;
        m_batch.brush.arg0 = center;
        m_batch.brush.arg1 = math::vec2(start.radians(), sweep_rad);

        return *this;
    }

    ui_draw_builder& ui_draw_builder::pattern_fill(pattern_fill_type type, math::angle a, float size0, float size1) noexcept
    {
        TAV_ASSERT(!m_has_fill_pattern);
        TAV_DEBUG_ONLY(m_has_fill_pattern = true);

        const auto c = math::cos(a.radians());
        const auto s = math::sin(a.radians());

        m_batch.pattern.flags = static_cast<uint32>(type);
        m_batch.pattern.cos_sin = math::pack_snorm2x16(c, s);
        m_batch.pattern.arg0 = math::vec2(size0, size1);

        return *this;
    }

    ui_draw_builder& ui_draw_builder::shadow(float bias, float radius) noexcept
    {
        m_batch.draw_shape.range_bias = math::pack_half2x16(radius, bias);
        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_shadow(float bias, float radius) noexcept
    {
        m_batch.cut_shape.range_bias = math::pack_half2x16(radius, bias);
        return *this;
    }

    ui_draw_builder& ui_draw_builder::layer(int32 z) noexcept
    {
        m_batch.sprite.z_index = z;
        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_quad(math::vec2 center, math::vec2 half_x, math::vec2 half_y) noexcept
    {
        TAV_ASSERT(!m_has_cut);
        TAV_DEBUG_ONLY(m_has_cut = true);

        m_batch.cut_shape.flags = static_cast<uint32>(shape_type::fill);
        m_batch.cut_shape.center = center;
        m_batch.cut_shape.half_x = half_x;
        m_batch.cut_shape.half_y = half_y;
        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_quad(geometry::aabb2 aabb) noexcept
    {
        return cut_quad(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f));
    }

    ui_draw_builder& ui_draw_builder::cut_quad(geometry::obb2 obb) noexcept
    {
        return cut_quad(obb.center, obb.right, obb.up);
    }

    ui_draw_builder& ui_draw_builder::cut_line(math::vec2 from, math::vec2 to, float thickness, float dash, float gap) noexcept
    {
        TAV_ASSERT(!m_has_cut);
        TAV_DEBUG_ONLY(m_has_cut = true);

        const math::vec2 half_dir = (to - from) * 0.5f;
        const float      inv_len = 1.0f / math::length(half_dir);
        m_batch.cut_shape.flags = static_cast<uint32>(shape_type::line);
        m_batch.cut_shape.center = (from + to) * 0.5f;
        m_batch.cut_shape.half_x = half_dir;
        m_batch.cut_shape.half_y = math::vec2(-half_dir.y * inv_len, half_dir.x * inv_len) * (thickness * 0.5f);
        m_batch.cut_shape.arg0.x = thickness;
        m_batch.cut_shape.arg1.x = dash;
        m_batch.cut_shape.arg1.y = gap;
        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash) noexcept
    {
        TAV_ASSERT(!m_has_cut);
        TAV_DEBUG_ONLY(m_has_cut = true);

        m_batch.cut_shape.flags = static_cast<uint32>(shape_type::lame_ellipse);
        m_batch.cut_shape.center = center;
        m_batch.cut_shape.half_x = half_x;
        m_batch.cut_shape.half_y = half_y;
        m_batch.cut_shape.arg0 = math::vec2(squash, 0.0f);
        m_batch.cut_shape.arg1 = math::vec2(0.0f, 0.0f);
        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_superellipse(geometry::aabb2 aabb, float squash) noexcept
    {
        return cut_superellipse(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), squash);
    }

    ui_draw_builder& ui_draw_builder::cut_superellipse(geometry::obb2 obb, float squash) noexcept
    {
        return cut_superellipse(obb.center, obb.right, obb.up, squash);
    }

    ui_draw_builder& ui_draw_builder::cut_rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius) noexcept
    {
        TAV_ASSERT(!m_has_cut);
        TAV_DEBUG_ONLY(m_has_cut = true);

        m_batch.cut_shape.flags = static_cast<uint32>(shape_type::round_rect);
        m_batch.cut_shape.center = center;
        m_batch.cut_shape.half_x = half_x;
        m_batch.cut_shape.half_y = half_y;
        m_batch.cut_shape.arg0 = math::vec2(radius.top_left, radius.top_right);
        m_batch.cut_shape.arg1 = math::vec2(radius.bottom_right, radius.bottom_left);
        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_rect(geometry::aabb2 aabb, geometry::radius4 radius) noexcept
    {
        return cut_rect(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), radius);
    }

    ui_draw_builder& ui_draw_builder::cut_rect(geometry::obb2 obb, geometry::radius4 radius) noexcept
    {
        return cut_rect(obb.center, obb.right, obb.up, radius);
    }

    ui_draw_builder& ui_draw_builder::cut_sdf(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        const auto sdf_size = static_cast<uint32>(math::ceil(math::abs(sdf_pad_size * k_sdf_shape_size_divisor)));

        TAV_ASSERT(sdf_size < k_sdf_max_pad_size);
        TAV_ASSERT(!m_has_cut);
        TAV_DEBUG_ONLY(m_has_cut = true);

        m_batch.cut_shape.flags = static_cast<uint32>(shape_type::sdf) | static_cast<uint32>(sdf_size << k_sdf_shift);
        m_batch.cut_shape.center = center;
        m_batch.cut_shape.half_x = half_x;
        m_batch.cut_shape.half_y = half_y;
        m_batch.cut_shape.arg0 = math::vec2(atlas_rect.x, atlas_rect.y);
        m_batch.cut_shape.arg1 = math::vec2(atlas_rect.z, atlas_rect.w);
        m_cut_sdf = tex;
        return *this;
    }

    ui_draw_builder& ui_draw_builder::cut_sdf(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        return cut_sdf(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), tex, atlas_rect, sdf_pad_size);
    }

    ui_draw_builder& ui_draw_builder::cut_sdf(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        return cut_sdf(obb.center, obb.right, obb.up, tex, atlas_rect, sdf_pad_size);
    }

    ui_draw_builder& ui_draw_builder::sprite(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept
    {
        TAV_ASSERT(!m_has_sprite);
        TAV_DEBUG_ONLY(m_has_sprite = true);

        m_batch.sprite.flags = static_cast<uint32>(k_has_sprite_flag);
        m_batch.sprite.center = center;
        m_batch.sprite.half_x = half_x;
        m_batch.sprite.half_y = half_y;
        m_batch.sprite.src_xy = math::vec2(sprite_rect.x, sprite_rect.y);
        m_batch.sprite.src_wh = math::vec2(sprite_rect.z, sprite_rect.w);
        m_draw_texture = tex;
        return *this;
    }

    ui_draw_builder& ui_draw_builder::sprite(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept
    {
        return sprite(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), tex, sprite_rect);
    }

    ui_draw_builder& ui_draw_builder::sprite(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept
    {
        return sprite(obb.center, obb.right, obb.up, tex, sprite_rect);
    }

    void ui_draw_builder::draw()
    {
        TAV_ASSERT(!m_drawn);
        TAV_DEBUG_ONLY(m_drawn = true);

        m_r->m_cmd->bind_pipeline(m_r->m_ui_mt->gpu_pipeline());
        m_r->m_cmd->push_constant(m_batch);
        auto                       b = m_r->m_paint_collection.upload(m_r->m_storage_buffer);
        const rhi::texture_binding bind_tex[3] = {
            rhi::texture_binding{m_draw_texture, m_r->m_rm->sampler(sampler_preset::nearest_repeat), 0},
            rhi::texture_binding{m_draw_sdf, m_r->m_rm->sampler(sampler_preset::linear_repeat), 1},
            rhi::texture_binding{m_cut_sdf, m_r->m_rm->sampler(sampler_preset::linear_repeat), 2},
        };
        m_r->m_cmd->bind_shader_textures(bind_tex);
        m_r->m_cmd->bind_shader_buffers(b);
        m_r->m_cmd->draw(6);

        m_r->m_paint_collection.clear(); // TODO: remove this line, it is just for testing
    }


    ui_renderer::ui_renderer(rhi::graphics_device* gdevice, resource_manager* rm)
        : m_gdevice(gdevice)
        , m_rm(rm)
    {
        init();
    }

    ui_renderer::~ui_renderer() noexcept
    {
        shutdown();
    }

    void ui_renderer::begin_frame() noexcept
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

    void ui_renderer::end_frame() noexcept
    {
        if (!m_cmd) {
            logger.error("Frame is not started.");
            return;
        }
        m_gdevice->submit_command_queue(m_cmd);
        m_cmd = nullptr;
    }

    ui_draw_builder ui_renderer::quad(math::vec2 center, math::vec2 half_x, math::vec2 half_y) noexcept
    {
        ui_draw_builder builder(this);
        builder.m_batch.draw_shape.flags = static_cast<uint32>(shape_type::fill);
        builder.m_batch.draw_shape.center = center;
        builder.m_batch.draw_shape.half_x = half_x;
        builder.m_batch.draw_shape.half_y = half_y;
        return builder;
    }

    ui_draw_builder ui_renderer::quad(geometry::aabb2 aabb) noexcept
    {
        return quad(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f));
    }

    ui_draw_builder ui_renderer::quad(geometry::obb2 obb) noexcept
    {
        return quad(obb.center, obb.right, obb.up);
    }

    ui_draw_builder ui_renderer::line(math::vec2 from, math::vec2 to, float thickness, float dash, float gap) noexcept
    {
        ui_draw_builder  builder(this);
        const math::vec2 half_dir = (to - from) * 0.5f;
        const float      inv_len = 1.0f / math::length(half_dir);
        builder.m_batch.draw_shape.flags = static_cast<uint32>(shape_type::line);
        builder.m_batch.draw_shape.center = (from + to) * 0.5f;
        builder.m_batch.draw_shape.half_x = half_dir;
        builder.m_batch.draw_shape.half_y = math::vec2(-half_dir.y * inv_len, half_dir.x * inv_len) * (thickness * 0.5f);
        builder.m_batch.draw_shape.arg0.x = thickness;
        builder.m_batch.draw_shape.arg1.x = dash;
        builder.m_batch.draw_shape.arg1.y = gap;
        return builder;
    }

    ui_draw_builder ui_renderer::superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash) noexcept
    {
        ui_draw_builder builder(this);
        builder.m_batch.draw_shape.flags = static_cast<uint32>(shape_type::lame_ellipse);
        builder.m_batch.draw_shape.center = center;
        builder.m_batch.draw_shape.half_x = half_x;
        builder.m_batch.draw_shape.half_y = half_y;
        builder.m_batch.draw_shape.arg0 = math::vec2(squash, 0.0f);
        return builder;
    }

    ui_draw_builder ui_renderer::superellipse(geometry::aabb2 aabb, float squash) noexcept
    {
        return superellipse(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), squash);
    }

    ui_draw_builder ui_renderer::superellipse(geometry::obb2 obb, float squash) noexcept
    {
        return superellipse(obb.center, obb.right, obb.up, squash);
    }

    ui_draw_builder ui_renderer::rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius) noexcept
    {
        ui_draw_builder builder(this);
        builder.m_batch.draw_shape.flags = static_cast<uint32>(shape_type::round_rect);
        builder.m_batch.draw_shape.center = center;
        builder.m_batch.draw_shape.half_x = half_x;
        builder.m_batch.draw_shape.half_y = half_y;
        builder.m_batch.draw_shape.arg0 = math::vec2(radius.top_left, radius.top_right);
        builder.m_batch.draw_shape.arg1 = math::vec2(radius.bottom_right, radius.bottom_left);
        return builder;
    }

    ui_draw_builder ui_renderer::rect(geometry::aabb2 aabb, geometry::radius4 radius) noexcept
    {
        return rect(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), radius);
    }

    ui_draw_builder ui_renderer::rect(geometry::obb2 obb, geometry::radius4 radius) noexcept
    {
        return rect(obb.center, obb.right, obb.up, radius);
    }

    ui_draw_builder ui_renderer::sdf(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle atlas, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        const auto sdf_size = static_cast<uint32>(math::ceil(math::abs(sdf_pad_size * k_sdf_shape_size_divisor)));
        TAV_ASSERT(sdf_size < k_sdf_max_pad_size);

        ui_draw_builder builder(this);
        builder.m_batch.draw_shape.flags = static_cast<uint32>(shape_type::sdf) | static_cast<uint32>(sdf_size << k_sdf_shift);
        builder.m_batch.draw_shape.center = center;
        builder.m_batch.draw_shape.half_x = half_x;
        builder.m_batch.draw_shape.half_y = half_y;
        builder.m_batch.draw_shape.arg0 = math::vec2(atlas_rect.x, atlas_rect.y);
        builder.m_batch.draw_shape.arg1 = math::vec2(atlas_rect.z, atlas_rect.w);
        builder.m_draw_sdf = atlas;
        return builder;
    }

    ui_draw_builder ui_renderer::sdf(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        return sdf(aabb.center(), math::vec2(aabb.width() * 0.5f, 0.0f), math::vec2(0.0f, aabb.height() * 0.5f), tex, atlas_rect, sdf_pad_size);
    }

    ui_draw_builder ui_renderer::sdf(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size) noexcept
    {
        return sdf(obb.center, obb.right, obb.up, tex, atlas_rect, sdf_pad_size);
    }

    void ui_renderer::init()
    {
        // TODO: change to buffer pool
        m_uniform_buffer.init(m_gdevice, 64 * 1024 * 1024, rhi::buffer_usage::constant, "ui_renderer.uniform_buffer");
        m_storage_buffer.init(m_gdevice, 64 * 1024 * 1024, rhi::buffer_usage::storage, "ui_renderer.storage_buffer");
        m_vertices_buffer.init(m_gdevice, 64 * 1024 * 1024, rhi::buffer_usage::vertex, "ui_renderer.vertices_buffer");
        m_rm->set_material_load_params({}, 1, rhi::pixel_format::none);

        m_ui_mt = m_rm->load_material("mt.uir");

        core::fixed_vector<material::vertex_attribute, 2> vert_attribs;
        vert_attribs.push_back(material::vertex_attribute{"a_gpyph_transform", sizeof(glyph_instance_data), offsetof(glyph_instance_data, transform), 1});
        vert_attribs.push_back(material::vertex_attribute{"a_bounds_and_color", sizeof(glyph_instance_data), offsetof(glyph_instance_data, rect), 1});
        m_rm->set_material_load_params(vert_attribs, 1, rhi::pixel_format::none);
        // m_text_mt = m_rm->load_material("mt.text2d");
    }

    void ui_renderer::shutdown() noexcept
    {
        m_uniform_buffer.shutdown();
        m_storage_buffer.shutdown();
        m_rm->release_material(m_ui_mt);
    }
} // namespace tavros::renderer

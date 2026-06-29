#include <tavros/renderer/renderer2d.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("renderer2d");

    namespace math = tavros::math;

    constexpr int32 k_solid_brush_type_id = 0;
    constexpr int32 k_linear_gradient_brush_type_id = 1;
    constexpr int32 k_radial_gradient_brush_type_id = 2;

    struct brush_data
    {
        math::vec2 pos0;
        math::vec2 pos1;
        math::vec4 color0;
        math::vec4 color1;
        int32      type;
    };

    // Normalize channel: 0..255 -> 0.0..1.0
    float norm_c(uint8 c) noexcept
    {
        return static_cast<float>(c) / 255.0f;
    }

    math::vec4 norm_color(math::rgba8 cl) noexcept
    {
        return math::vec4(norm_c(cl.r), norm_c(cl.g), norm_c(cl.b), norm_c(cl.a));
    }
} // namespace

namespace tavros::renderer
{

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

    void renderer2d::set_brush_solid_color(math::rgba8 color)
    {
        auto       cl = norm_color(color);
        const auto bd = brush_data{{0.0f, 0.0f}, {0.0f, 0.0f}, cl, cl, k_solid_brush_type_id};
        auto       slice = m_uniform_buffer.slice<brush_data>(1);
        slice.data().copy_from(&bd, 1);
        m_cmd->bind_shader_buffers(rhi::buffer_binding{slice.gpu_buffer(), static_cast<uint32>(slice.offset_bytes()), static_cast<uint32>(slice.size_bytes()), 1});
    }

    void renderer2d::set_brush_linear_gradient(math::vec2 start, math::rgba8 start_color, math::vec2 end, math::rgba8 end_color)
    {
        auto       cl1 = norm_color(start_color);
        auto       cl2 = norm_color(end_color);
        const auto bd = brush_data{start, end, cl1, cl2, k_linear_gradient_brush_type_id};
        auto       slice = m_uniform_buffer.slice<brush_data>(1);
        slice.data().copy_from(&bd, 1);
        m_cmd->bind_shader_buffers(rhi::buffer_binding{slice.gpu_buffer(), static_cast<uint32>(slice.offset_bytes()), static_cast<uint32>(slice.size_bytes()), 1});
    }

    void renderer2d::set_brush_radial_gradient(math::vec2 center, math::rgba8 center_color, float radius, math::rgba8 end_color)
    {
        auto       cl1 = norm_color(center_color);
        auto       cl2 = norm_color(end_color);
        const auto bd = brush_data{center, {radius, 0.0f}, cl1, cl2, k_radial_gradient_brush_type_id};
        auto       slice = m_uniform_buffer.slice<brush_data>(1);
        slice.data().copy_from(&bd, 1);
        m_cmd->bind_shader_buffers(rhi::buffer_binding{slice.gpu_buffer(), static_cast<uint32>(slice.offset_bytes()), static_cast<uint32>(slice.size_bytes()), 1});
    }

    void renderer2d::set_line_thickness(float thickness) noexcept
    {
        m_line_thickness = thickness;
    }

    void renderer2d::set_line_dash(float dash, float gap) noexcept
    {
        m_line_dash = dash;
        m_line_gap = gap;
    }

    void renderer2d::line(math::vec2 p0, math::vec2 p1)
    {
        TAV_ASSERT(m_cmd);
        if (!m_cmd) {
            logger.error("Failed to draw line: frame is not started.");
            return;
        }

        struct line_data
        {
            math::vec2 p0;
            math::vec2 p1;
            float      thickness;
            float      aa_width;
            float      dash_size;
            float      gap_size;
        };

        line_data l{p0, p1, m_line_thickness, m_aa_width, m_line_dash, m_line_gap};
        m_cmd->bind_pipeline(m_line_mt->gpu_pipeline());
        m_cmd->push_constant(l);
        m_cmd->draw(6);

        move_to(p1);
    }

    void renderer2d::move_to(math::vec2 p)
    {
        m_pen_pos = p;
    }

    void renderer2d::line_to(math::vec2 p)
    {
        line(m_pen_pos, p);
    }

    void renderer2d::polyline(core::buffer_view<math::vec2> points)
    {
        if (points.empty()) {
            return;
        }

        move_to(points[0]);
        for (size_t i = 1; i < points.size(); ++i) {
            line_to(points[i]);
        }
    }

    void renderer2d::set_circle_dash(float dash, float gap) noexcept
    {
        m_circle_dash = dash;
        m_circle_gap = gap;
    }

    void renderer2d::circle(math::vec2 center, float radius)
    {
        ring_cut(center, radius, 0.0f, math::vec2{0.0f, 0.0f});
    }

    void renderer2d::ring(math::vec2 center, float outer_radius, float inner_radius)
    {
        ring_cut(center, outer_radius, inner_radius, math::vec2{0.0f, 0.0f});
    }

    void renderer2d::ring_cut(math::vec2 center, float outer_radius, float inner_radius, math::vec2 inner_offset)
    {
        TAV_ASSERT(m_cmd);
        if (!m_cmd) {
            logger.error("Failed to draw ring: frame is not started.");
            return;
        }

        struct circle_data
        {
            math::vec2 center;
            math::vec2 outer_center;
            float      outer_r;
            float      inner_r;
            float      dash_size;
            float      gap_size;
            float      aa_width;
        };

        circle_data c{center, center + inner_offset, outer_radius, inner_radius, m_line_dash, m_line_gap, m_aa_width};
        m_cmd->bind_pipeline(m_circle_mt->gpu_pipeline());
        m_cmd->push_constant(c);
        m_cmd->draw(6);
    }

    void renderer2d::rect(math::vec2 center, math::vec2 half_sizes, math::vec4 radius)
    {
        rect_cut(center, half_sizes, {0.0f, 0.0f}, {0.0f, 0.0f}, radius);
    }

    void renderer2d::rect_cut(math::vec2 center, math::vec2 outer_half_sizes, math::vec2 inner_offset, math::vec2 inner_half_sizes, math::vec4 outer_radius, math::vec4 inner_radius)
    {
        TAV_ASSERT(m_cmd);
        if (!m_cmd) {
            logger.error("Failed to draw rect: frame is not started.");
            return;
        }

        struct rect_data
        {
            math::vec2 center;
            math::vec2 inner_center;
            math::vec2 outer_half_sizes;
            math::vec2 inner_half_sizes;
            math::vec4 outer_radius;
            math::vec4 inner_radius;
            float      aa_width;
        };

        float outer_max_r = math::min(outer_half_sizes.x, outer_half_sizes.y);
        outer_radius = math::min(math::vec4(outer_max_r), outer_radius);
        float inner_max_r = math::min(inner_half_sizes.x, inner_half_sizes.y);
        inner_radius = math::min(math::vec4(outer_max_r), inner_radius);

        rect_data r{center, center + inner_offset, outer_half_sizes, inner_half_sizes, outer_radius, inner_radius, m_aa_width};
        m_cmd->bind_pipeline(m_rect_mt->gpu_pipeline());
        m_cmd->push_constant(r);
        m_cmd->draw(6);
    }

    void renderer2d::set_sprite_pivot(math::vec2 pivot) noexcept
    {
        m_sprite_pivot = pivot;
    }

    void renderer2d::set_sprite_sampler(sampler_preset smp) noexcept
    {
        m_sprite_smp = smp;
    }

    void renderer2d::sprite(texture_ref texture, math::vec2 pos, math::vec2 size, float rot, math::vec4 src_rect)
    {
        TAV_ASSERT(m_cmd);
        if (!m_cmd) {
            logger.error("Failed to draw sprite: frame is not started.");
            return;
        }

        struct sprite_data
        {
            math::vec4 src_rect;
            math::vec2 pos;
            math::vec2 size;
            math::vec2 pivot;
            float      rot;
        };

        auto s = sprite_data{src_rect, pos, size, m_sprite_pivot, rot};
        auto smp = m_rm->samplers().get_sampler(m_sprite_smp);
        m_cmd->bind_pipeline(m_sprite_mt->gpu_pipeline());
        m_cmd->bind_shader_textures(rhi::texture_binding{texture->gpu_texture(), smp, 0});
        m_cmd->push_constant(s);
        m_cmd->draw(6);
    }

    void renderer2d::init()
    {
        // TODO: change to buffer pool
        m_uniform_buffer.init(m_gdevice, 64 * 1024 * 1024, rhi::buffer_usage::constant);
        m_line_mt = m_rm->load<material>("mt.line2d");
        m_rect_mt = m_rm->load<material>("mt.rect2d");
        m_circle_mt = m_rm->load<material>("mt.circle2d");
        m_sprite_mt = m_rm->load<material>("mt.sprite2d");
    }

    void renderer2d::shutdown() noexcept
    {
        m_uniform_buffer.shutdown();
        m_rm->release(m_line_mt);
        m_rm->release(m_rect_mt);
        m_rm->release(m_circle_mt);
        m_rm->release(m_sprite_mt);
    }
} // namespace tavros::renderer

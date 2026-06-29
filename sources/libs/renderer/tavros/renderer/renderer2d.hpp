#pragma once

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/resource_manager.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>
#include <tavros/core/math.hpp>

namespace tavros::renderer
{

    class renderer2d
    {
    public:
        renderer2d(rhi::graphics_device* gdevice, resource_manager* rm);

        ~renderer2d() noexcept;

        void begin_frame() noexcept;
        void end_frame() noexcept;

        void set_brush_solid_color(math::rgba8 color);
        void set_brush_linear_gradient(math::vec2 start, math::rgba8 start_color, math::vec2 end, math::rgba8 end_color);
        void set_brush_radial_gradient(math::vec2 center, math::rgba8 center_color, float radius, math::rgba8 end_color);

        void set_line_thickness(float thickness) noexcept;
        void set_line_dash(float dash, float gap) noexcept;

        void line(math::vec2 p0, math::vec2 p1);
        void move_to(math::vec2 p);
        void line_to(math::vec2 p);
        void polyline(core::buffer_view<math::vec2> points);

        void set_circle_dash(float dash, float gap) noexcept;

        void circle(math::vec2 center, float radius);
        void ring(math::vec2 center, float outer_radius, float inner_radius);
        void ring_cut(math::vec2 center, float outer_radius, float inner_radius, math::vec2 inner_offset);

        void rect(math::vec2 center, math::vec2 half_sizes, math::vec4 radius = {0.0f, 0.0f, 0.0f, 0.0f});
        void rect_cut(math::vec2 center, math::vec2 outer_half_sizes, math::vec2 inner_offset, math::vec2 inner_half_sizes, math::vec4 outer_radius = {0.0f}, math::vec4 inner_radius = {0.0f});

        void set_sprite_pivot(math::vec2 pivot) noexcept;
        void set_sprite_sampler(sampler_preset smp) noexcept;

        void sprite(texture_ref texture, math::vec2 pos, math::vec2 size, float rot, math::vec4 src_rect);

    private:
        void init();
        void shutdown() noexcept;

    private:
        rhi::graphics_device* m_gdevice = nullptr;
        resource_manager*     m_rm = nullptr;

        tavros::renderer::gpu_stream_buffer m_uniform_buffer;

        rhi::command_queue* m_cmd = nullptr;

        material_ref m_line_mt;
        material_ref m_rect_mt;
        material_ref m_circle_mt;
        material_ref m_sprite_mt;

        math::vec2     m_pen_pos;
        math::vec2     m_sprite_pivot;
        sampler_preset m_sprite_smp = sampler_preset::automatic;

        float m_aa_width = 0.5f;
        float m_line_thickness = 1.0f;
        float m_line_dash = 0.0f;
        float m_line_gap = 0.0f;
        float m_circle_dash = 0.0f;
        float m_circle_gap = 0.0f;
    };

} // namespace tavros::renderer

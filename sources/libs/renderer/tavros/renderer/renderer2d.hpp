#pragma once

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/resource_manager.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>
#include <tavros/renderer/text/rich_text.hpp>
#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>

#include <tavros/renderer/shader_bindings/paint_collection.hpp>
#include <tavros/renderer/shader_bindings/brush_collection.hpp>
#include <tavros/renderer/shader_bindings/shape_collection.hpp>

namespace tavros::renderer
{
    class renderer2d;
    
    enum class gradient_repeat : uint8
    {
        clamp = 0,
        repeat = 1,
        mirror = 2
    };

    class draw_builder
    {
    public:
        [[nodiscard]] draw_builder& fill_color(math::color32 color) noexcept;
        [[nodiscard]] draw_builder& fill_linear(core::buffer_view<color_stop> gradient, math::vec2 from, math::vec2 to, gradient_repeat repeat = gradient_repeat::clamp) noexcept;
        [[nodiscard]] draw_builder& fill_radial(core::buffer_view<color_stop> gradient, math::vec2 center, float radius, gradient_repeat repeat = gradient_repeat::clamp) noexcept;
        [[nodiscard]] draw_builder& fill_conic(core::buffer_view<color_stop> gradient, math::vec2 center, float start_angle_rad, gradient_repeat repeat = gradient_repeat::clamp) noexcept;

        [[nodiscard]] draw_builder& cut_line(math::vec2 from, math::vec2 to, float thickness = 0.0f, float dash = 0.0f, float gap = 0.0f);
        [[nodiscard]] draw_builder& cut_superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash);
        [[nodiscard]] draw_builder& cut_rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius = {});

        [[nodiscard]] draw_builder& sprite(texture_ref texture, math::vec2 center, math::vec2 half_x, math::vec2 half_y, math::vec4 sprite_rect);

        void draw();
    private:
        friend class renderer2d;

		void line(math::vec2 from, math::vec2 to, float thickness, float dash, float gap) noexcept;
		void superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash) noexcept;
		void rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius) noexcept;

        draw_builder(renderer2d* r) noexcept;
        ~draw_builder() noexcept = default;

    private:
        
        struct brush_t
        {
            uint32 flags = 0;    // Type
            uint32 first = 0;    // first color index
            uint32 num = 0;      // number of colors
            uint32 color = 0;    // Only if num == 1 (solid brush)
            math::vec2 arg0; // start point (linear) / center (radial)
            math::vec2 arg1; // end point (linear) / arg1.x - radius (radial)
        };

        struct shape_t
        {
            uint32     flags = 0;
            uint32     reserved = 0;
            math::vec2 center; 
            math::vec2 x_axis; // x axis half size 
            math::vec2 y_axis; // y axis half size
            math::vec2 arg0;  // LINE: x - thickness; CIRCLE: x - x_axis radius, y - y_axis radius; RECT: x - radius tl, y - radius tr
            math::vec2 arg1;  // LINE: x - dash, y - gap; CIRCLE: NONE; RECT: x - radius br, y - radius bl
        };

        struct sprite_t // 48 bytes
        {
            uint32     flags = 0;
        
            uint32     reserved = 0;
            math::vec2 center;
            math::vec2 x_axis; // x axis half size 
            math::vec2 y_axis; // y axis half size
            math::vec4 src_rect;
        };

        struct push_constants
        {
            brush_t  brush;
            shape_t  draw_shape;
            shape_t  cut_shape;
            sprite_t sprite;
        };

    private:
        renderer2d* m_r = nullptr;

        push_constants m_pc;
    };


    class renderer2d
    {
    public:
        renderer2d(rhi::graphics_device* gdevice, resource_manager* rm);

        ~renderer2d() noexcept;

        void begin_frame() noexcept;
        void end_frame() noexcept;


        [[nodiscard]] draw_builder line(math::vec2 from, math::vec2 to, float thickness = 0.0f, float dash = 0.0f, float gap = 0.0f);
        [[nodiscard]] draw_builder superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash = 1.0f);
        [[nodiscard]] draw_builder rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius = {});

        






        
        //void draw_text(const text_archetype& text, math::vec2 pos, float fill_treshold = 0.5f, float outline_treshold = 0.0f);

        //void set_clip_rect(geometry::aabb2 aabb) noexcept;
        //void reset_clip_rect() noexcept;

    private:
        void init();
        void shutdown() noexcept;

		friend class draw_builder;

    private:
        rhi::graphics_device* m_gdevice = nullptr;
        resource_manager*     m_rm = nullptr;

        paint_collection m_paint_collection;
        brush_collection m_brush_collection;
        shape_collection m_shape_collection;

        tavros::renderer::gpu_stream_buffer m_storage_buffer;
        tavros::renderer::gpu_stream_buffer m_uniform_buffer;
        tavros::renderer::gpu_stream_buffer m_vertices_buffer;

        rhi::command_queue* m_cmd = nullptr;

        //material_ref m_line_mt;
        //material_ref m_rect_mt;
        //material_ref m_circle_mt;
        /*material_ref m_sprite_mt;
        material_ref m_text_mt;*/

        material_ref m_r2d_mt;

       /* math::vec2     m_pen_pos;
        math::vec2     m_sprite_pivot;
        sampler_preset m_sprite_smp = sampler_preset::linear_clamp;
        sampler_preset m_text_smp = sampler_preset::linear_clamp;
        bool           m_text_use_fill_mask = false;
        bool           m_text_use_outline_mask = false;*/

       /* float m_aa_width = 0.5f;
        float m_line_thickness = 1.0f;
        float m_line_dash = 0.0f;
        float m_line_gap = 0.0f;
        float m_circle_dash = 0.0f;
        float m_circle_gap = 0.0f;*/
    };

} // namespace tavros::renderer

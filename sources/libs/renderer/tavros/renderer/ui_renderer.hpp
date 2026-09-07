#pragma once

#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/resource_manager.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>
#include <tavros/renderer/text/rich_text.hpp>
#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>
#include <tavros/core/debug/debug.hpp>

#include <tavros/renderer/shader_bindings/paint_collection.hpp>
#include <tavros/renderer/shader_bindings/brush_collection.hpp>
#include <tavros/renderer/shader_bindings/shape_collection.hpp>

namespace tavros::renderer
{
    class ui_renderer;

    enum class gradient_repeat : uint8
    {
        clamp = 0,
        repeat = 1,
        mirror = 2
    };

    enum class pattern_fill_type : uint8
    {
        none = 0,
        rect = 1,
        hexagon = 2,
        brick = 3,
        voronoi = 4,
        cairo = 5,
        stripes = 6,
    };

    class ui_draw_builder
    {
    public:
        ~ui_draw_builder() noexcept;

        [[nodiscard]] ui_draw_builder& fill_color(math::color32 color) noexcept;
        [[nodiscard]] ui_draw_builder& fill_linear(core::buffer_view<color_stop> gradient, math::vec2 from, math::vec2 to, gradient_repeat repeat = gradient_repeat::clamp) noexcept;
        [[nodiscard]] ui_draw_builder& fill_radial(core::buffer_view<color_stop> gradient, math::vec2 center, float radius, gradient_repeat repeat = gradient_repeat::clamp) noexcept;
        [[nodiscard]] ui_draw_builder& fill_conic(core::buffer_view<color_stop> gradient, math::vec2 center, math::angle start = math::angle(0.0f), math::angle sweep = math::angle(0.0f), gradient_repeat repeat = gradient_repeat::clamp) noexcept;

        [[nodiscard]] ui_draw_builder& pattern_fill(pattern_fill_type type, math::angle a, float size0, float size1 = 0.0f) noexcept;

        [[nodiscard]] ui_draw_builder& shadow(float bias, float radius) noexcept;
        [[nodiscard]] ui_draw_builder& cut_shadow(float bias, float radius) noexcept;

        [[nodiscard]] ui_draw_builder& layer(int32 z) noexcept;


        [[nodiscard]] ui_draw_builder& cut_quad(math::vec2 center, math::vec2 half_x, math::vec2 half_y) noexcept;
        [[nodiscard]] ui_draw_builder& cut_quad(geometry::aabb2 aabb) noexcept;
        [[nodiscard]] ui_draw_builder& cut_quad(geometry::obb2 obb) noexcept;

        [[nodiscard]] ui_draw_builder& cut_line(math::vec2 from, math::vec2 to, float thickness = 0.0f, float dash = 0.0f, float gap = 0.0f) noexcept;

        [[nodiscard]] ui_draw_builder& cut_superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash) noexcept;
        [[nodiscard]] ui_draw_builder& cut_superellipse(geometry::aabb2 aabb, float squash) noexcept;
        [[nodiscard]] ui_draw_builder& cut_superellipse(geometry::obb2 obb, float squash) noexcept;

        [[nodiscard]] ui_draw_builder& cut_rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius = {}) noexcept;
        [[nodiscard]] ui_draw_builder& cut_rect(geometry::aabb2 aabb, geometry::radius4 radius = {}) noexcept;
        [[nodiscard]] ui_draw_builder& cut_rect(geometry::obb2 obb, geometry::radius4 radius = {}) noexcept;

        [[nodiscard]] ui_draw_builder& cut_sdf(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0) noexcept;
        [[nodiscard]] ui_draw_builder& cut_sdf(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0) noexcept;
        [[nodiscard]] ui_draw_builder& cut_sdf(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0) noexcept;

        [[nodiscard]] ui_draw_builder& sprite(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept;
        [[nodiscard]] ui_draw_builder& sprite(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept;
        [[nodiscard]] ui_draw_builder& sprite(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 sprite_rect) noexcept;

        void draw();

    private:
        friend class ui_renderer;

        ui_draw_builder(ui_renderer* r) noexcept;

    private:
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

        struct draw_batch
        {
            brush_t   brush;
            pattern_t pattern;
            shape_t   draw_shape;
            shape_t   cut_shape;
            sprite_t  sprite; // z index stored here
        };

        rhi::texture_handle m_draw_texture;
        rhi::texture_handle m_draw_sdf;
        rhi::texture_handle m_cut_sdf;

    private:
        ui_renderer* m_r = nullptr;
        draw_batch   m_batch;

        debug::debug_only<bool> m_drawn = false;
        debug::debug_only<bool> m_has_fill = false;
        debug::debug_only<bool> m_has_fill_pattern = false;
        debug::debug_only<bool> m_has_cut = false;
        debug::debug_only<bool> m_has_sprite = false;
    };


    class ui_renderer
    {
    public:
        ui_renderer(rhi::graphics_device* gdevice, resource_manager* rm);
        ~ui_renderer() noexcept;

        void begin_frame() noexcept;
        void end_frame() noexcept;

        [[nodiscard]] ui_draw_builder quad(math::vec2 center, math::vec2 half_x, math::vec2 half_y) noexcept;
        [[nodiscard]] ui_draw_builder quad(geometry::aabb2 aabb) noexcept;
        [[nodiscard]] ui_draw_builder quad(geometry::obb2 obb) noexcept;

        [[nodiscard]] ui_draw_builder line(math::vec2 from, math::vec2 to, float thickness, float dash = 0.0f, float gap = 0.0f) noexcept;

        [[nodiscard]] ui_draw_builder superellipse(math::vec2 center, math::vec2 half_x, math::vec2 half_y, float squash) noexcept;
        [[nodiscard]] ui_draw_builder superellipse(geometry::aabb2 aabb, float squash) noexcept;
        [[nodiscard]] ui_draw_builder superellipse(geometry::obb2 obb, float squash) noexcept;

        [[nodiscard]] ui_draw_builder rect(math::vec2 center, math::vec2 half_x, math::vec2 half_y, geometry::radius4 radius) noexcept;
        [[nodiscard]] ui_draw_builder rect(geometry::aabb2 aabb, geometry::radius4 radius = {}) noexcept;
        [[nodiscard]] ui_draw_builder rect(geometry::obb2 obb, geometry::radius4 radius = {}) noexcept;

        [[nodiscard]] ui_draw_builder sdf(math::vec2 center, math::vec2 half_x, math::vec2 half_y, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0f) noexcept;
        [[nodiscard]] ui_draw_builder sdf(geometry::aabb2 aabb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0f) noexcept;
        [[nodiscard]] ui_draw_builder sdf(geometry::obb2 obb, rhi::texture_handle tex, math::vec4 atlas_rect, float sdf_pad_size = 1.0f) noexcept;

        //[[nodiscard]] ui_draw_builder text(const text_archetype& text, uint32 first, uint32 size, rhi::texture_handle atlas);

    private:
        void init();
        void shutdown() noexcept;

        friend class ui_draw_builder;

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

        material_ref m_ui_mt;
    };

} // namespace tavros::renderer

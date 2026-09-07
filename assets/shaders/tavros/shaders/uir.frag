#include <tavros/scene/brush.glsl>
#include <tavros/scene/shape.glsl>
#include <tavros/scene/spatial.glsl>
#include <tavros/scene/sprite.glsl>

layout(PUSH_CONSTANT) uniform PushConstants
{
    brush_t     brush;      // 32 bytes
    partition_t pattern;    // 16 bytes
    shape_t     draw_shape; // 48 bytes
    shape_t     cut_shape;  // 48 bytes
    sprite_t    sprite;     // 48 bytes
} pc; // 192 bytes

layout(binding = 0) uniform sampler2D u_sprite;
layout(binding = 1) uniform sampler2D u_draw_shape;
layout(binding = 2) uniform sampler2D u_cut_shape;

in vec2 v_world_pos;

in vec2 v_sprite_uv;
in vec4 v_sprite_scaled_rect;

in vec2 v_draw_shape_uv;
in vec4 v_draw_shape_scaled_rect;

in vec2 v_cut_shape_uv;
in vec4 v_cut_shape_scaled_rect;


out vec4 base_color;

void main()
{
    vec2 pos = v_world_pos;
    float draw_alpha = sample_shape_alpha(pc.draw_shape, pos, 1.0, u_draw_shape, v_draw_shape_uv, v_draw_shape_scaled_rect);
    float cut_alpha = sample_shape_alpha(pc.cut_shape, pos, 1.0, u_cut_shape, v_cut_shape_uv, v_cut_shape_scaled_rect);

    float alpha = draw_alpha - cut_alpha;

    if (alpha <= 0.0) {
        discard;
    }

    vec2 tiled_pos = apply_tiling_pattern(pc.pattern, pos);
    vec4 brush_smp = sample_brush(pc.brush, tiled_pos);
    vec4 brush_color = vec4(brush_smp.rgb, brush_smp.a * alpha);

    vec2 sampled_uv = sample_repeat(v_sprite_uv, v_sprite_scaled_rect.xy, v_sprite_scaled_rect.zw);
    vec4 spr_color = sample_sprite(pc.sprite, u_sprite, sampled_uv);

    base_color = brush_color * spr_color;
}

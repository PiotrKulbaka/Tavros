#include <tavros/scene/brush.glsl>
#include <tavros/scene/shape.glsl>

#define HAS_SPRITE 0x01

struct sprite_t // 48 bytes
{
    uint  flags;
    uint  reserved;
    vec2  center;
    vec2  x_axis; // x axis half size 
    vec2  y_axis; // y axis half size
    vec2  src_xy; // source sprite xy
    vec2  src_wh; // source sprite wh
};

layout(PUSH_CONSTANT) uniform PushConstants
{
    brush_t brush;      // 32 bytes
    shape_t draw_shape; // 48 bytes
    shape_t cut_shape;  // 48 bytes
    sprite_t sprite;    // 48 bytes
} pc; // 176 bytes

layout(binding = 0) uniform sampler2D u_texture;

in vec2 v_world_pos;
in vec2 v_uv;
in vec4 v_scaled_rect;

out vec4 base_color;

vec2 sample_repeat(vec2 uv, vec2 rect_xy, vec2 rect_wh)
{
    vec2 tiled_uv = mod(uv, 1.0);
    vec2 tile_offset = floor(uv);
    return vec2(tile_offset + rect_xy + tiled_uv * rect_wh);
}

void main()
{
    vec2 pos = v_world_pos;
    float draw_alpha = sample_shape_alpha(pc.draw_shape, pos, 1.0, 0.0);
    float cut_alpha = sample_shape_alpha(pc.cut_shape, pos, 1.0, 0.0);

    float alpha = draw_alpha - cut_alpha;

    if (alpha <= 0.0) {
        discard;
    }

    vec4 brush_color = sample_brush(pc.brush, pos);
    vec4 color = vec4(brush_color.rgb, brush_color.a * alpha);

    if ((pc.sprite.flags & HAS_SPRITE) == HAS_SPRITE) {
        vec2 sampled_uv = sample_repeat(v_uv, v_scaled_rect.xy, v_scaled_rect.zw);
        base_color = color * texture(u_texture, sampled_uv);
    } else {
        base_color = color;
    }
}

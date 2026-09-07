#include <tavros/scene/scene.glsl>
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


vec2 get_vertex(const in shape_t s, const in int vertex_id)
{
    const vec2 corners[6] = vec2[6](
        vec2(-1.0, -1.0), // 0: bottom-left
        vec2( 1.0, -1.0), // 1: bottom-right
        vec2( 1.0,  1.0), // 2: top-right
        vec2(-1.0, -1.0), // 3: bottom-left
        vec2( 1.0,  1.0), // 4: top-right
        vec2(-1.0,  1.0)  // 5: top-left
    );
    vec2 c = corners[vertex_id];

    uint shape_type = tav_shape_type(s);
    if (TAV_SHAPE_TYPE_SDF == shape_type) {
        vec2 world_pos = mat2(s.x_axis, s.y_axis) * c + s.center;
        return world_pos;
    }

    vec2 range_bias = unpackHalf2x16(s.range_bias);
    float margin = range_bias.x + range_bias.y;

    vec2 x_dir = (margin > 0.0) ? normalize(s.x_axis) : vec2(0.0);
    vec2 y_dir = (margin > 0.0) ? normalize(s.y_axis) : vec2(0.0);

    vec2 world_pos = mat2(s.x_axis, s.y_axis) * c + s.center;
    world_pos += x_dir * (margin * c.x) + y_dir * (margin * c.y);
    return world_pos;
}



out vec2 v_world_pos;

out vec2 v_sprite_uv;
out vec4 v_sprite_scaled_rect;

out vec2 v_draw_shape_uv;
out vec4 v_draw_shape_scaled_rect;

out vec2 v_cut_shape_uv;
out vec4 v_cut_shape_scaled_rect;

void main()
{
    vec2 pos = get_vertex(pc.draw_shape, gl_VertexID);
    v_world_pos = pos;

    if ((pc.sprite.flags & HAS_SPRITE) == HAS_SPRITE) {
        vec2 local = inverse(mat2(pc.sprite.x_axis, pc.sprite.y_axis)) * (pos - pc.sprite.center);
        v_sprite_uv = local * 0.5 + 0.5;

        vec2 tex_size = vec2(textureSize(u_sprite, 0));
        v_sprite_scaled_rect = vec4(pc.sprite.src_xy / tex_size, pc.sprite.src_wh / tex_size);
    }

    uint draw_shape_type = tav_shape_type(pc.draw_shape);
    if (TAV_SHAPE_TYPE_SDF == draw_shape_type) {
        vec2 local = tav_shape_to_local(pc.draw_shape, pos);
        v_draw_shape_uv = local * 0.5 + 0.5;

        vec2 tex_size = vec2(textureSize(u_draw_shape, 0));
        v_draw_shape_scaled_rect = vec4(pc.draw_shape.arg0 / tex_size, pc.draw_shape.arg1 / tex_size);
    }

    uint cut_shape_type = tav_shape_type(pc.cut_shape);
    if (TAV_SHAPE_TYPE_SDF == cut_shape_type) {
        vec2 local = tav_shape_to_local(pc.cut_shape, pos);
        v_cut_shape_uv = local * 0.5 + 0.5;

        vec2 tex_size = vec2(textureSize(u_cut_shape, 0));
        v_cut_shape_scaled_rect = vec4(pc.cut_shape.arg0 / tex_size, pc.cut_shape.arg1 / tex_size);
    }

    gl_Position = ui_to_clip(pos);
}

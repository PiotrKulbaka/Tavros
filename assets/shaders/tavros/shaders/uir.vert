#include <tavros/scene/scene.glsl>
#include <tavros/scene/brush.glsl>
#include <tavros/scene/shape.glsl>
#include <tavros/scene/spatial.glsl>
#include <tavros/scene/sprite.glsl>

struct draw_command_t
{
    brush_t     brush;      // 32 bytes
    partition_t pattern;    // 16 bytes
    shape_t     draw_shape; // 48 bytes
    shape_t     cut_shape;  // 48 bytes
    sprite_t    sprite;     // 48 bytes
    scissor_t   scissor;    // 16 bytes
}; // 208 bytes

layout(std430, binding = 2) buffer DrawBuffer
{
    draw_command_t draws[];
} db;



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

out flat uint v_instance_id;

void main()
{
    uint instance_index = uint(gl_InstanceID) + uint(gl_BaseInstance);
    v_instance_id = instance_index;
    draw_command_t cmd = db.draws[instance_index];

    vec2 pos = get_vertex(cmd.draw_shape, gl_VertexID);
    v_world_pos = pos;

    if (tav_has_sprite(cmd.sprite)) {
        vec2 local = inverse(mat2(cmd.sprite.x_axis, cmd.sprite.y_axis)) * (pos - cmd.sprite.center);
        v_sprite_uv = local * 0.5 + 0.5;

        vec2 tex_size = vec2(textureSize(u_sprite, 0));
        v_sprite_scaled_rect = vec4(cmd.sprite.src_xy / tex_size, cmd.sprite.src_wh / tex_size);
    }

    if (tav_has_mask(cmd.draw_shape)) {
        vec2 local = tav_shape_to_local(cmd.draw_shape, pos);
        v_draw_shape_uv = local * 0.5 + 0.5;

        vec2 tex_size = vec2(textureSize(u_draw_shape, 0));
        v_draw_shape_scaled_rect = vec4(cmd.draw_shape.arg0 / tex_size, cmd.draw_shape.arg1 / tex_size);
    }

    if (tav_has_mask(cmd.cut_shape)) {
        vec2 local = tav_shape_to_local(cmd.cut_shape, pos);
        v_cut_shape_uv = local * 0.5 + 0.5;

        vec2 tex_size = vec2(textureSize(u_cut_shape, 0));
        v_cut_shape_scaled_rect = vec4(cmd.cut_shape.arg0 / tex_size, cmd.cut_shape.arg1 / tex_size);
    }

    gl_Position = ui_to_clip(pos);
}

#include <tavros/scene/scene.glsl>
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

out vec2 v_world_pos;
out vec2 v_uv;
out vec4 v_scaled_rect;

void main()
{
    vec2 pos = get_shape_vertex(pc.draw_shape, gl_VertexID);
    v_world_pos = pos;

    if ((pc.sprite.flags & HAS_SPRITE) == HAS_SPRITE) {
        mat2 basis = mat2(pc.sprite.x_axis, pc.sprite.y_axis);
        vec2 local = inverse(basis) * (pos - pc.sprite.center);
        vec2 uv_pos = local * 0.5 + 0.5;

        vec2 tex_size = vec2(textureSize(u_texture, 0));
        tex_size = vec2(tex_size.x, -tex_size.y); // Inverse Y
        v_scaled_rect = vec4(pc.sprite.src_xy / tex_size, pc.sprite.src_wh / tex_size);
        v_uv = uv_pos;
    }

    gl_Position = ui_to_clip(pos);
}

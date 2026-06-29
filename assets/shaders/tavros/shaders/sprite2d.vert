#include <tavros/scene/scene.glsl>

layout(PUSH_CONSTANT) uniform PushConstants
{
    vec4  src_rect;
    vec2  pos;
    vec2  size;
    vec2  pivot;
    float rot_rad;
} pc;

out vec2 v_uv;
out vec2 v_coord;

layout(binding = 0) uniform sampler2D u_texture;

void main()
{
    vec2 pos = k_quad[gl_VertexID];
    
    vec2 pivot = pc.pivot * 2.0 - 1.0;
    vec2 local = (pos - pivot) * pc.size * 0.5;

    float s = sin(pc.rot_rad);
    float c = cos(pc.rot_rad);
    vec2 rotated = vec2(local.x * c - local.y * s, local.x * s + local.y * c);

    vec2 world_pos = pc.pos + rotated;
    v_coord = world_pos;

    vec2 tex_size = vec2(textureSize(u_texture, 0));
    vec2 uv_min = pc.src_rect.xy / tex_size;
    vec2 uv_max = uv_min + pc.src_rect.zw / tex_size;

    vec2 uv_pos = pos * 0.5 + 0.5;
    v_uv = uv_min + uv_pos * (uv_max - uv_min);
    v_uv.y = 1.0 - v_uv.y;

    gl_Position = scene.ortho_projection * vec4(world_pos, 0.0, 1.0);
}
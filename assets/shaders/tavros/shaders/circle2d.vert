#include <tavros/scene/scene.glsl>

layout(PUSH_CONSTANT) uniform PushConstants
{
    vec2  center;
    vec2  inner_center;
    float outer_r;
    float inner_r;
    float dash_size;
    float gap_size;
    float aa_width;
} pc;

out vec2 v_coord;

void main()
{
    vec2 pos = k_quad[gl_VertexID];
    vec2 world_pos = pc.center + pos * pc.outer_r;
    v_coord = world_pos;
    gl_Position = scene.ortho_projection * vec4(world_pos, 0.0, 1.0);
}

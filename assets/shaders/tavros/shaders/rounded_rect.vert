#include <tavros/scene/scene.glsl>

layout(PUSH_CONSTANT) uniform PushConstants
{
    vec2  center;
    vec2  inner_center;
    vec2  outer_half_sizes; // left-right, top-bottom
    vec2  inner_half_sizes; // left-right, top-bottom
    vec4  outer_radius; // left-top, right-top, right-bottom, left-bottom
    vec4  inner_radius; // left-top, right-top, right-bottom, left-bottom
    float aa_width;
} pc;

out vec2 v_coord;

void main()
{
    vec2 pos = k_quad[gl_VertexID];
    vec2 world_pos = pc.center + pos * pc.outer_half_sizes;
    v_coord = world_pos;
    gl_Position = scene.ortho_projection * vec4(world_pos, 0.0, 1.0);
}
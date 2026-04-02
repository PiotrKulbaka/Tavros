#include <tavros/shaders/scene.glsl>

const vec2 k_quad[6] = vec2[6](
    vec2(-1, -1), vec2( 1, -1), vec2( 1,  1),
    vec2(-1, -1), vec2( 1,  1), vec2(-1,  1)
);

layout(PUSH_CONSTANT) uniform PushConstants
{
    vec2  pos;
    float out_r; // outsize radius
    float in_r; // inside radius
    vec4  color;
    float aa_width;
    float dash_size;
    float gap_size;
} pc;

out vec2 v_coord;

void main()
{
    vec2 pos = k_quad[gl_VertexID];
    float r = pc.out_r + pc.aa_width;
    vec2 pixel_pos = pc.pos + pos * r;
    v_coord = pixel_pos;
    gl_Position = scene.ortho_projection * vec4(pixel_pos, 0.0, 1.0);
}

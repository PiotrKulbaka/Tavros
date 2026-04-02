#include <tavros/shaders/scene.glsl>

const vec2 k_quad[6] = vec2[6](
    vec2(-1, -1), vec2( 1, -1), vec2( 1,  1),
    vec2(-1, -1), vec2( 1,  1), vec2(-1,  1)
);

layout(PUSH_CONSTANT) uniform PushConstants
{
    vec2  p0;
    vec2  p1;
    vec4  color;
    float thickness;
    float aa_width;
    float dash_size;
    float gap_size;
} pc;

out vec2 v_coord;

void main()
{
    vec2 pos = k_quad[gl_VertexID];

    vec2 a = pc.p0;
    vec2 b = pc.p1;

    vec2 dir  = normalize(b - a);
    vec2 perp = vec2(-dir.y, dir.x);

    float half_len = length(b - a) * 0.5;
    float half_w   = (pc.thickness * 0.5) + pc.aa_width;

    vec2 center = (a + b) * 0.5;

    vec2 pixel_pos = center
        + dir  * pos.x * (half_len + half_w)
        + perp * pos.y * half_w;

    v_coord = pixel_pos;

    gl_Position = scene.ortho_projection * vec4(pixel_pos, 0.0, 1.0);
}

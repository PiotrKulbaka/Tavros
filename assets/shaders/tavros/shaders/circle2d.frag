#include <tavros/shaders/scene.glsl>

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

in  vec2 v_coord;
out vec4 frag_color;

float sdf_ring(vec2 p, vec2 center, float r_outer, float r_inner)
{
    float d = length(p - center);
    return max(r_inner - d, d - r_outer);
}

void main()
{
    float dist = sdf_ring(v_coord, pc.pos, pc.out_r, pc.in_r);
    float aa = pc.aa_width;
    float alpha = 1.0 - smoothstep(-aa, aa, dist);

    float angle = atan(v_coord.y - pc.pos.y, v_coord.x - pc.pos.x);
    float t = mod(angle * pc.out_r, pc.dash_size + pc.gap_size);

    if (t > pc.dash_size)
        discard;

    if (alpha <= 0.0) {
        discard;
    }

    frag_color = vec4(pc.color.rgb, pc.color.a * alpha);
}

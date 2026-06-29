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

in vec2 v_coord;

out vec4 base_color;

// r: corner radius - tl, tr, br, bl
float sd_round_box(vec2 p, vec2 half_size, vec4 r)
{
    // x > 0 = right side: pick tr/br, else tl/bl
    r.xy = (p.x > 0.0) ? r.yz : r.xw;
    // y > 0 = bottom: pick br/bl, else tr/tl
    r.x  = (p.y > 0.0) ? r.y  : r.x;

    vec2 q = abs(p) - half_size + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

void main()
{
    vec2 p = v_coord - pc.center;
    float outer = sd_round_box(p, pc.outer_half_sizes, pc.outer_radius);
    float outer_alpha = 1.0 - smoothstep(-pc.aa_width, pc.aa_width, outer);

    vec2 p2 = v_coord - pc.inner_center;
    float inner = sd_round_box(p2, pc.inner_half_sizes, pc.inner_radius);
    float inner_alpha = smoothstep(-pc.aa_width, pc.aa_width, inner);

    float alpha = outer_alpha * inner_alpha;

    vec4 color = sample_brush(v_coord);
    base_color = vec4(color.rgb, color.a * alpha);
}
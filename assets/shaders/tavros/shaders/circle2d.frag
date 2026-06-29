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

in  vec2 v_coord;
out vec4 base_color;

void main()
{
    float aa = pc.aa_width;

    // Outer
    float outer_dist = length(v_coord - pc.center) - pc.outer_r;
    float outer_alpha = 1.0 - smoothstep(-aa, aa, outer_dist);

    // Inner
    float inner_dist = length(v_coord - pc.inner_center) - pc.inner_r;
    float inner_alpha = smoothstep(-aa, aa, inner_dist);

    float alpha = outer_alpha * inner_alpha;

    // Dash
    if (pc.dash_size > 0.0) {
        float angle = atan(v_coord.y - pc.center.y, v_coord.x - pc.center.x);
        float t = mod(angle * pc.outer_r, pc.dash_size + pc.gap_size);
        float dash_alpha = 1.0 - smoothstep(pc.dash_size - aa, pc.dash_size + aa, t);
        alpha *= dash_alpha;
    }

    vec4 color = sample_brush(v_coord);
    base_color = vec4(color.rgb, color.a * alpha);
}

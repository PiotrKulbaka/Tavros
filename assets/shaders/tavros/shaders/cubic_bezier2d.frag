// https://iquilezles.org/articles/bezierbbox/

#include <tavros/scene/scene.glsl>

layout(PUSH_CONSTANT) uniform PushConstants
{
    vec2  p0;
    vec2  p1;
    vec2  p2;
    vec2  p3;
    vec4  color;
    float thickness;
    float aa_width;
    float dash_size;
    float gap_size;
} pc;

in  vec2 v_coord;
out vec4 base_color;

// Returns vec2(distance, t) -- closest point on cubic bezier to p
// Uses linear tessellation. 20 steps is enough for smooth curves at typical screen sizes.
vec3 sdf_bezier(vec2 p, vec2 p0, vec2 p1, vec2 p2, vec2 p3)
{
    const int k_steps = 36;

    float best_dist = 1e10;
    float best_t = 0.0;
    float best_len = 0.0;

    float accum_len = 0.0;
    vec2 prev = p0;

    for (int i = 1; i <= k_steps; ++i) {
        float t = float(i) / float(k_steps);
        float s = 1.0 - t;

        vec2 curr = s * s * s * p0 + 3.0 * s * s * t * p1 + 3.0 * s * t * t * p2 + t * t * t * p3;
        vec2 seg = curr - prev;
        float seg_len = length(seg);
        vec2 rel = p - prev;
        float seg_len_sq = dot(seg, seg);
        float u = seg_len_sq > 1e-10
            ? clamp(dot(rel, seg) / seg_len_sq, 0.0, 1.0)
            : 0.0;

        vec2 closest = prev + u * seg;
        float d = length(p - closest);

        if (d < best_dist) {
            best_dist = d;
            best_t = mix(float(i - 1), float(i), u) / float(k_steps);
            best_len = accum_len + seg_len * u;
        }

        accum_len += seg_len;
        prev = curr;
    }

    return vec3(best_dist, best_t, best_len);
}

void main()
{
    vec3 result = sdf_bezier(v_coord, pc.p0, pc.p1, pc.p2, pc.p3);

    float dist = result.x;
    float arc_len = result.z;

    float dash_alpha = 1.0;
    if (pc.dash_size > 0.0) {
        float period = pc.dash_size + pc.gap_size;
        float phase = mod(arc_len, period);

        dash_alpha = 1.0 - smoothstep(pc.dash_size - pc.aa_width, pc.dash_size + pc.aa_width, phase);
    }

    float half_w = pc.thickness * 0.5;
    float alpha = 1.0 - smoothstep(half_w - pc.aa_width, half_w + pc.aa_width, dist);

    alpha *= dash_alpha;

    base_color = vec4(pc.color.rgb, pc.color.a * alpha);
}

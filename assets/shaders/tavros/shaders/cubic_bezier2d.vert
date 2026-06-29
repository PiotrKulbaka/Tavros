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

out vec2 v_coord;

// Returns vec4(aabb_min.xy, aabb_max.xy) for a cubic bezier in world space
vec4 bezier_aabb(vec2 p0, vec2 p1, vec2 p2, vec2 p3)
{
    vec2 c = -p0 +       p1;
    vec2 b =  p0 - 2.0 * p1 +       p2;
    vec2 a = -p0 + 3.0 * p1 - 3.0 * p2 + p3;

    // Derivative roots: a*t^2 + b*t + c = 0 (per component)
    vec2 disc = b * b - a * c;
    vec2 g    = sqrt(max(disc, vec2(0.0)));

    vec2 t1 = clamp((-b - g) / a, 0.0, 1.0);
    vec2 t2 = clamp((-b + g) / a, 0.0, 1.0);

    // Evaluate bezier at t1, t2 using Horner form: p0 + t*(3c + t*(3b + t*a))
    vec2 q1 = p0 + t1 * (3.0 * c + t1 * (3.0 * b + t1 * a));
    vec2 q2 = p0 + t2 * (3.0 * c + t2 * (3.0 * b + t2 * a));

    return vec4(
        min(min(p0, p3), min(q1, q2)),
        max(max(p0, p3), max(q1, q2))
    );
}

void main()
{
    float pad = pc.thickness * 0.5 + pc.aa_width;

    vec4 box = bezier_aabb(pc.p0, pc.p1, pc.p2, pc.p3);

    // Expand AABB by half-thickness + aa padding
    vec2 aabb_min = box.xy - vec2(pad);
    vec2 aabb_max = box.zw + vec2(pad);

    // Map unit quad [0,1]^2 to expanded AABB in world space
    vec2 pos = k_quad[gl_VertexID];
    vec2 pixel_pos = mix(aabb_min, aabb_max, pos);

    v_coord = pixel_pos;

    gl_Position = scene.ortho_projection * vec4(pixel_pos, 0.0, 1.0);
}

#include <tavros/shaders/scene.glsl>

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

in  vec2 v_coord;
out vec4 frag_color;

float sdf_segment(vec2 p, vec2 a, vec2 b)
{
    vec2  ab = b - a;
    vec2  ap = p - a;
    float t  = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
    return length(ap - ab * t);
}

void main()
{
    float dist   = sdf_segment(v_coord, pc.p0, pc.p1);
    if (pc.dash_size > 0.1f) {
        vec2  ab      = pc.p1 - pc.p0;
        vec2  ap      = v_coord - pc.p0;
        float t       = dot(ap, normalize(ab));
        float period  = pc.dash_size + pc.gap_size;
        float phase   = mod(t, period);
        if (phase > pc.dash_size) discard;
    }
    float half_w = pc.thickness * 0.5;
    float alpha  = 1.0 - smoothstep(half_w - pc.aa_width, half_w + pc.aa_width, dist);

    if (alpha <= 0.0) discard;

    frag_color = vec4(pc.color.rgb, pc.color.a * alpha);
}

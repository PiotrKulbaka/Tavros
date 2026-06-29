#include <tavros/scene/scene.glsl>

layout(PUSH_CONSTANT) uniform PushConstants
{
    vec2  p0;
    vec2  p1;
    float thickness;
    float aa_width;
    float dash_size;
    float gap_size;
} pc;

in  vec2 v_coord;
out vec4 base_color;

float sdf_segment(vec2 p, vec2 a, vec2 b)
{
    vec2  ab = b - a;
    vec2  ap = p - a;
    float t  = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
    return length(ap - ab * t);
}

void main()
{
    vec2 a = pc.p0;
    vec2 b = pc.p1;

    float aa_width = pc.aa_width;
    float dist   = sdf_segment(v_coord, a, b);
    if (pc.dash_size > 0.1f) {
        vec2  ab      = b - a;
        vec2  ap      = v_coord - a;
        float t       = dot(ap, normalize(ab));
        float period  = pc.dash_size + pc.gap_size;
        float phase   = mod(t, period);
        if (phase > pc.dash_size) discard;
    }
    float half_w = pc.thickness * 0.5;
    float alpha  = 1.0 - smoothstep(half_w - aa_width, half_w + aa_width, dist);

    
    vec4 color = sample_brush(v_coord);
    base_color = vec4(color.rgb, color.a * alpha);
}

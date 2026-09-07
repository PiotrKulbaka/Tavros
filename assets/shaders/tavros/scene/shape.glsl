#ifndef _TAV_SHAPE_GLSL_
#define _TAV_SHAPE_GLSL_

#include <tavros/scene/sprite.glsl>

#define TAV_SHAPE_TYPE_NONE          0
#define TAV_SHAPE_TYPE_FILL          1
#define TAV_SHAPE_TYPE_LINE          2
#define TAV_SHAPE_TYPE_LAME_ELLIPSE  3
#define TAV_SHAPE_TYPE_ROUND_RECT    4
#define TAV_SHAPE_TYPE_SDF           5
#define TAV_SHAPE_TYPE_MASK          0xFF
#define TAV_SHAPE_SDF_SIZE_MASK      0xFFFF
#define TAV_SHAPE_SDF_SIZE_SHIFT     16
#define TAV_SHAPE_SDF_SIZE_DIVISOR   64.0

struct shape_t // 48 bytes
{
    uint flags; // shape type
    uint range_bias;  
    vec2 center; 
    vec2 x_axis; // half-extent along local X
    vec2 y_axis; // half-extent along local Y
    vec2 arg0;   // LINE: x - thickness; RECT: x - radius tl, y - radius tr; SDF: atlas XY
    vec2 arg1;   // LINE: x - dash, y - gap; RECT: x - radius br, y - radius bl; SDF: atlas WH
};

// r: corner radius - tl, tr, br, bl
float sd_round_box(vec2 p, vec2 half_size, vec4 r)
{
    r.xy = (p.x > 0.0) ? r.yz : r.xw;
    r.x  = (p.y > 0.0) ? r.y  : r.x;

    vec2 q = abs(p) - half_size + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

float sd_squash(vec2 p, float k)
{
    float n = pow(2.0, k);
    p = abs(p);
    p = max(p, vec2(1e-5));
    float f = pow(p.x, n) + pow(p.y, n) - 1.0;
    vec2 grad = n * pow(p, vec2(n - 1.0));
    return f / max(length(grad), 1e-6);
}

uint tav_shape_type(const in shape_t s)
{
    return s.flags & TAV_SHAPE_TYPE_MASK;
}

float tav_shape_sdf_size(const in shape_t s)
{
    return float((s.flags >> TAV_SHAPE_SDF_SIZE_SHIFT) & TAV_SHAPE_SDF_SIZE_MASK) / TAV_SHAPE_SDF_SIZE_DIVISOR;
}

vec2 tav_shape_to_local(const in shape_t s, const in vec2 world_pos)
{
    return inverse(mat2(s.x_axis, s.y_axis)) * (world_pos - s.center);
}

vec2 tav_shape_to_local_rotated(const in shape_t s, const in vec2 world_pos)
{
    vec2 d = world_pos - s.center;
    return vec2(dot(d, normalize(s.x_axis)), dot(d, normalize(s.y_axis)));
}

float sample_shape_alpha(const in shape_t s, const in vec2 p, const in float aa_scale, sampler2D sdf_tex, vec2 sdf_uv, vec4 sdf_rect)
{
    uint shape_type = tav_shape_type(s);
    if (TAV_SHAPE_TYPE_NONE == shape_type) {
        return 0.0;
    }

    vec2 range_bias = unpackHalf2x16(s.range_bias);
    float range = range_bias.x;
    float bias  = range_bias.y;

    float sd = 0.0;

    switch (shape_type) {
        case TAV_SHAPE_TYPE_FILL:
        {
            vec2 half_extents = vec2(length(s.x_axis), length(s.y_axis));
            vec2 local_p = tav_shape_to_local_rotated(s, p);

            vec2 q = abs(local_p) - half_extents;
            sd = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0);
            break;
        }

        case TAV_SHAPE_TYPE_LINE:
        {
            vec2 a = s.center - s.x_axis;
            vec2 b = s.center + s.x_axis;

            float thickness = s.arg0.x;
            float dash_size = s.arg1.x;
            float gap_size = s.arg1.y;

            vec2  ab = b - a;
            vec2  ap = p - a;
            float dot_ap_ab = dot(ap, ab);
            float t_d  = clamp(dot_ap_ab / max(dot(ab, ab), 1e-6), 0.0, 1.0);
            float dist = length(ap - ab * t_d);

            float half_w = thickness * 0.5;
            sd = dist - half_w;

            if (dash_size > 0.0) {
                float len = length(ab);
                if (len > 0.0) {
                    float t = dot_ap_ab / len;
                    float period = dash_size + gap_size;
                    float phase = mod(t, period);

                    float dash_dist = abs(phase - dash_size * 0.5) - dash_size * 0.5;
                    if (dash_dist > 0.0) {
                        sd = max(sd, dash_dist);
                    }
                }
            }
            break;
        }

        case TAV_SHAPE_TYPE_LAME_ELLIPSE:
        {
            vec2 half_extents = vec2(length(s.x_axis), length(s.y_axis));
            vec2 local_p = tav_shape_to_local(s, p);
            float squash_k = s.arg0.x;
            sd = sd_squash(local_p, squash_k) * min(half_extents.x, half_extents.y);
            break;
        }

        case TAV_SHAPE_TYPE_ROUND_RECT:
        {
            vec2 half_extents = vec2(length(s.x_axis), length(s.y_axis));
            vec2 local_p = tav_shape_to_local_rotated(s, p);
            vec4 radii = vec4(s.arg0, s.arg1);
            sd = sd_round_box(local_p, half_extents, radii);
            break;
        }

        case TAV_SHAPE_TYPE_SDF:
        {
            vec2 sampled_uv = sample_repeat(sdf_uv, sdf_rect.xy, sdf_rect.zw);
            float raw = texture(sdf_tex, sampled_uv).r;

            float scaled_sdf_size = tav_shape_sdf_size(s);
            bias = clamp(bias, -scaled_sdf_size, scaled_sdf_size);
            float max_range = scaled_sdf_size - abs(bias);
            range = clamp(range, 0.0, max_range);
    
            sd = raw * scaled_sdf_size;
            break;
        }

        default:
            return 0.5; // FOR DEBUG
    }

    float pixel_aa = fwidth(sd) * aa_scale * 0.5;

    return 1.0 - smoothstep(-pixel_aa, range + pixel_aa, sd - bias);
}

#endif // _TAV_SHAPE_GLSL_

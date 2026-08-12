#define SHAPE_TYPE_NONE          0
#define SHAPE_TYPE_FILL          1
#define SHAPE_TYPE_LINE          2
#define SHAPE_TYPE_LAME_ELLIPSE  3
#define SHAPE_TYPE_ROUND_RECT    4
#define SHAPE_TYPE_MASK          0x0F

struct shape_t
{
    uint  flags; // shape type
    uint  reserved;
    vec2  center;
    vec2  x_axis; // x axis half size 
    vec2  y_axis; // y axis half size
    vec2  arg0;  // LINE: x - thickness; RECT: x - radius tl, y - radius tr
    vec2  arg1;  // LINE: x - dash, y - gap; RECT: x - radius br, y - radius bl
};

// // r: corner radius - tl, tr, br, bl
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

float sample_shape_alpha(const in shape_t s, const in vec2 p, const in float aa_scale,const in float bias)
{
    uint shape_type = s.flags & SHAPE_TYPE_MASK;
    if (SHAPE_TYPE_NONE == shape_type) {
        return 0.0;
    }

    if (SHAPE_TYPE_FILL == shape_type) {
        return 1.0;
    }

    float sd = 0.0;

    if (SHAPE_TYPE_LINE == shape_type) {
        // ================ LINE ================
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

    } else if (SHAPE_TYPE_LAME_ELLIPSE == shape_type) {
        // ================ LAME ELLIPSE ================
        vec2 center = s.center;
        float rx = length(s.x_axis);
        float ry = length(s.y_axis);
        float squash_k = s.arg0.x;

        vec2 local_p = p - center;
        vec2 x_dir = normalize(s.x_axis);
        vec2 y_dir = normalize(s.y_axis);
        local_p = vec2(dot(local_p, x_dir), dot(local_p, y_dir));
        local_p /= vec2(rx, ry);
    
        sd = sd_squash(local_p, squash_k) * min(rx, ry);

    } else if (SHAPE_TYPE_ROUND_RECT == shape_type) {
        // ================ ROUND RECT ================
        vec2 half_size = vec2(length(s.x_axis), length(s.y_axis));

        vec2 local_p = p - s.center;
        vec2 x_dir = normalize(s.x_axis);
        vec2 y_dir = normalize(s.y_axis);
        local_p = vec2(dot(local_p, x_dir), dot(local_p, y_dir)); // Only if rotated
        
        vec4 radii = vec4(s.arg0.x, s.arg0.y, s.arg1.x, s.arg1.y);
        sd = sd_round_box(local_p, half_size, radii);

    } else {
        return 0.5; // FOR DEBUG
    }

    float aa = fwidth(length(p)) * aa_scale * 0.5;

    return 1.0 - smoothstep(-aa, aa, sd - bias);
}

vec2 get_shape_vertex(const in shape_t s, const in uint index) {
    const vec2 corners[6] = vec2[6](
        vec2(-1.0, -1.0), // 0: bottom-left
        vec2( 1.0, -1.0), // 1: bottom-right
        vec2( 1.0,  1.0), // 2: top-right
        vec2(-1.0, -1.0), // 3: bottom-left
        vec2( 1.0,  1.0), // 4: top-right
        vec2(-1.0,  1.0)  // 5: top-left
    );
    
    vec2 v = corners[index];

    vec2 vertex = s.center + v.x * s.x_axis + v.y * s.y_axis;
    return vertex;
}

// layout(std430, binding = 3) buffer ShapeBuffer
// {
//     uint size;     // Size of shapes[]
//     uint reserved; // Reserved for future
//     shape_t shapes[];
// } shape_buffer;
//
// float sample_shape(const in uint shape_id, const in vec2 p, const in float aa_width)
// {
//     if (0 == shape_id) {
//         return 1.0f;
//     }
//
//     uint index = max(shape_id, shape_buffer.size);
//     shape_t s = shape_buffer.shapes[index];
//
//     return sample_shape(s, p, aa_width);
// }

#ifndef _TAV_BRUSH_GLSL_
#define _TAV_BRUSH_GLSL_

#define BRUSH_TYPE_WHITE_COLOR      0
#define BRUSH_TYPE_LINEAR_GRADIENT  1
#define BRUSH_TYPE_RADIAL_GRADIENT  2
#define BRUSH_TYPE_CONIC_GRADIENT   3
#define BRUSH_TYPE_MASK             0x0F
#define BRUSH_TYPE_SHIFT            0u

#define BRUSH_REPEAT_CLAMP           0
#define BRUSH_REPEAT_REPEAT          1
#define BRUSH_REPEAT_MIRROR          2
#define BRUSH_REPEATING_MASK        0xF0
#define BRUSH_REPEATING_SHIFT       4u


struct color_stop_t
{
    float stop;         // Normalized stop index
    uint  packed_color; // Packed color
};

struct brush_t
{
    uint flags;
    uint first; // first color index
    uint num;   // number of colors
    uint color; // Only if num == 1 (solid brush)
    vec2 arg0;  // start point (linear) / center (radial)
    vec2 arg1;  // end point (linear) / arg1.x - radius (radial)
};

layout(std430, binding = 1) buffer PaintBuffer
{
    uint size;     // Size of paints[]
    uint reserved; // Reserved for future
    color_stop_t paints[];
} paint_buffer;



color_stop_t fetch_stop(const in brush_t b, uint i)
{
    return paint_buffer.paints[b.first + i];
}

float apply_repeating(float t, uint mode)
{
    if (mode == BRUSH_REPEAT_REPEAT) {
        return fract(t);
    } else if (mode == BRUSH_REPEAT_MIRROR) {
        float tt = fract(t * 0.5) * 2.0;
        return 1.0 - abs(tt - 1.0);
    }

    return clamp(t, 0.0, 1.0); // BRUSH_REPEAT_CLAMP
}

vec4 sample_brush(const in brush_t b, const in vec2 p)
{
    uint brush_type = (b.flags >> BRUSH_TYPE_SHIFT) & BRUSH_TYPE_MASK;
    if (BRUSH_TYPE_WHITE_COLOR == brush_type || 0 == b.num) {
        return vec4(1.0f);
    }

    if (1u == b.num) {
        return unpackUnorm4x8(b.color);
    }

    vec2 local_pos = p - b.arg0;

    float t;
    if (brush_type == BRUSH_TYPE_LINEAR_GRADIENT) {
        vec2  axis    = b.arg1 - b.arg0; // Vector start to end
        float axis_sq = dot(axis, axis);
        t = dot(local_pos, axis) / max(axis_sq, 1e-6); // proj in [0,1]
    } else if (brush_type == BRUSH_TYPE_RADIAL_GRADIENT) {
        float radius = max(b.arg1.x, 1e-6);
        t = length(local_pos) / radius; // proj in [0,1]
    } else { // BRUSH_TYPE_CONIC_GRADIENT
        const float TWO_PI = 6.28318530717958647692;
        float start_angle = b.arg1.x;
        float sweep_rad   = b.arg1.y;
        float raw_angle = atan(local_pos.y, local_pos.x);
        float angle = mod(raw_angle - start_angle, TWO_PI);
        t = angle / sweep_rad;
    }

    uint repeat_mode = (b.flags & BRUSH_REPEATING_MASK) >> BRUSH_REPEATING_SHIFT;
    t = apply_repeating(t, repeat_mode);

    uint num = b.num;

    color_stop_t first_stop = fetch_stop(b, 0u);
    color_stop_t last_stop  = fetch_stop(b, num - 1u);

    if (repeat_mode == BRUSH_REPEAT_REPEAT && (t < first_stop.stop || t > last_stop.stop)) {

        float span = (first_stop.stop + 1.0) - last_stop.stop;
        span = max(span, 1e-6);

        float local_t = (t > last_stop.stop)
            ? (t - last_stop.stop) / span
            : (t + 1.0 - last_stop.stop) / span;

        vec4 c0 = unpackUnorm4x8(last_stop.packed_color);
        vec4 c1 = unpackUnorm4x8(first_stop.packed_color);

        return mix(c0, c1, local_t);
    }

    uint lo = 0u;
    uint hi = num - 1u;

    while (hi - lo > 1u) {
        uint mid = (lo + hi) >> 1u;
        if (fetch_stop(b, mid).stop <= t) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    color_stop_t s0 = fetch_stop(b, lo);
    color_stop_t s1 = fetch_stop(b, lo + 1u);

    float denom = max(s1.stop - s0.stop, 1e-6);
    float local_t = (t - s0.stop) / denom;

    vec4 c0 = unpackUnorm4x8(s0.packed_color);
    vec4 c1 = unpackUnorm4x8(s1.packed_color);

    return mix(c0, c1, local_t);
}

#endif // _TAV_BRUSH_GLSL_



// layout(std430, binding = 2) buffer BrushBuffer
// {
//     uint size;           // Size of brushes
//     uint no_brush_color; // Color for brush ID 0
//     uint reserved0;      // Reserved for future
//     uint reserved1;      // Reserved for future
//     brush_t brushes[];
// } brush_buffer;
//
// vec4 sample_brush(const in uint brush_id, const in vec2 p)
// {
//     uint index = min(brush_id, uint(brush_buffer.size));
//
//     if (0u == index) {
//         return unpackUnorm4x8(brush_buffer.no_brush_color);
//     }
//
//     brush_t b = brush_buffer.brushes[index];
//     return sample_brush(b, p);
// }

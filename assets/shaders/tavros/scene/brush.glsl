#define BRUSH_MAX_COLORS            16
#define BRUSH_TYPE_LINEAR_GRADIENT  0
#define BRUSH_TYPE_RADIAL_GRADIENT  1
#define BRUSH_TYPE_MASK             0x3

struct brush_t // 152 bytes
{
    uint  flags;                    // Brush type, one of BRUSH_TYPE_LINEAR_GRADIENT, BRUSH_TYPE_RADIAL_GRADIENT
    uint  num;                      // Number of colors, max value BRUSH_MAX_COLORS
    vec2  pos;                      // start point for gradients
    vec2  dir;                      // normalized direction for gradients
    float stops[BRUSH_MAX_COLORS];  // Soprted stops
    uint  colors[BRUSH_MAX_COLORS]; // Packed rgba colors
};

vec4 sample_brush(const in brush_t b, in vec2 p)
{
    uint num = min(b.num, uint(BRUSH_MAX_COLORS));

    if (0 == num) {
        return vec4(1.0);
    }

    if (1 == num) {
        return unpackUnorm4x8(b.colors[0]);
    }

    uint brush_type = b.flags & BRUSH_TYPE_MASK;

    vec2 local_pos = p - b.pos;
    float t = (brush_type == BRUSH_TYPE_LINEAR_GRADIENT)
        ? dot(local_pos, b.dir)
        : length(local_pos); // BRUSH_TYPE_RADIAL_GRADIENT

    t = clamp(t, b.stops[0], b.stops[num - 1u]);

    uint lo = 0u;
    uint hi = num - 1u;

    while (hi - lo > 1u) {
        uint mid = (lo + hi) >> 1u;
        if (b.stops[mid] <= t) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    float stop0 = b.stops[lo];
    float stop1 = b.stops[lo + 1u];

    float denom = max(stop1 - stop0, 1e-6);
    float local_t = (t - stop0) / denom;

    vec4 c0 = unpackUnorm4x8(b.colors[lo]);
    vec4 c1 = unpackUnorm4x8(b.colors[lo + 1u]);

    return mix(c0, c1, local_t);
}

// layout (std140, binding = 1) uniform Brush
// {
//     brush_t b;
// } brush;

// vec4 sample_brush_(in vec2 p)
// {
//     return sample_brush(brush.b, p);
// }

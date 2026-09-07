#ifndef _TAV_SPATIAL_GLSL_
#define _TAV_SPATIAL_GLSL_

struct partition_t // spatial partition
{
    uint  flags;
    uint  cs;   // packed cos/sin  
    vec2  arg0;
}; // 4 * 4 = 16 bytes

#define TILING_NONE         0u
#define TILING_SQUARE       1u
#define TILING_HEXAGON      2u
#define TILING_BRICK        3u
#define TILING_VORONOI      4u
#define TILING_CAIRO        5u
#define TILING_STRIPES      6u



vec2 hex_center(const in vec2 p, float hex_size)
{
    const float SQRT3 = 1.7320508075688772;
    vec2 q = vec2(p.x * (2.0/3.0) / hex_size, (-p.x/3.0 + SQRT3/3.0*p.y) / hex_size);

    vec3 cube = vec3(q.x, -q.x - q.y, q.y);
    vec3 rounded = round(cube);
    vec3 diff = abs(rounded - cube);

    if (diff.x > diff.y && diff.x > diff.z) {
        rounded.x = -rounded.y - rounded.z;
    } else if (diff.y > diff.z) {
        rounded.y = -rounded.x - rounded.z;
    } else {
        rounded.z = -rounded.x - rounded.y;
    }

    vec2 axial = vec2(rounded.x, rounded.z);
    return vec2(hex_size * 1.5 * axial.x, hex_size * SQRT3 * (axial.y + axial.x * 0.5));
}

vec2 tile_center_brick(const in vec2 p, vec2 tile_size)
{
    float row = floor(p.y / tile_size.y);
    float offset = mod(row, 2.0) * (tile_size.x * 0.5);
    float col = floor((p.x - offset) / tile_size.x);

    vec2 center;
    center.x = col * tile_size.x + offset + tile_size.x * 0.5;
    center.y = row * tile_size.y + tile_size.y * 0.5;

    return center;
}

vec2 hash2(vec2 p)
{
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453123);
}

vec2 voronoi_center(const in vec2 p, float cell_size)
{
    vec2 g = p / cell_size;
    vec2 i_cell = floor(g);
    vec2 f_cell = fract(g);

    float min_dist = 1e10;
    vec2 closest_center = vec2(0.0);

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(i_cell + neighbor);
            vec2 diff = neighbor + point - f_cell;

            float dist = dot(diff, diff);
            if (dist < min_dist) {
                min_dist = dist;
                closest_center = (i_cell + neighbor + point) * cell_size;
            }
        }
    }

    return closest_center;
}

vec2 cairo_tile_center(const in vec2 p, float size)
{
    const float SQRT3 = 1.7320508075688772;

    vec2 q = vec2(p.x * (2.0/3.0) / size, (-p.x/3.0 + SQRT3/3.0*p.y) / size);
    vec3 cube = vec3(q.x, -q.x - q.y, q.y);
    vec3 rnd = round(cube);
    vec3 diff = abs(rnd - cube);

    if (diff.x > diff.y && diff.x > diff.z) {
        rnd.x = -rnd.y - rnd.z;
    } else if (diff.y > diff.z) {
        rnd.y = -rnd.x - rnd.z;
    } else {
        rnd.z = -rnd.x - rnd.y;
    }

    vec2 hex_center = vec2(size * 1.5 * rnd.x, size * SQRT3 * (rnd.z + rnd.x * 0.5));

    vec2 local = p - hex_center;
    float angle = atan(local.y, local.x);
    float sector = floor((angle + 3.14159265) / (3.14159265 / 3.0)); // 0..5

    float sector_angle = sector * (3.14159265 / 3.0) - 3.14159265 + (3.14159265 / 6.0);
    vec2 offset = vec2(cos(sector_angle), sin(sector_angle)) * size * 0.5;

    return hex_center + offset;
}

vec2 tile_center_square(const in vec2 p, vec2 tile_size)
{
    vec2 cell = floor(p / tile_size);
    return (cell + 0.5) * tile_size;
}

vec2 tile_center_horizontal(const in vec2 p, float size)
{
    float row = floor(p.y / size);
    return vec2(p.x, (row + 0.5) * size);
}



vec2 apply_tiling_pattern(const in partition_t pt, const in vec2 pos)
{
    uint type = pt.flags;

    if (TILING_NONE == type) {
        return pos;
    }

    vec2 cs = unpackSnorm2x16(pt.cs);
    mat2 m = mat2(cs.x, -cs.y, cs.y, cs.x);
    mat2 im = mat2(cs.x, cs.y, -cs.y, cs.x);

    switch (type)
    {
        case TILING_SQUARE:
            return tile_center_square(pos * m, pt.arg0) * im;

        case TILING_HEXAGON:
            return hex_center(pos * m, pt.arg0.x) * im;

        case TILING_BRICK:
            return tile_center_brick(pos * m, pt.arg0) * im;

        case TILING_VORONOI:
            return voronoi_center(pos * m, pt.arg0.x) * im;

        case TILING_CAIRO:
            return cairo_tile_center(pos * m, pt.arg0.x) * im;

        case TILING_STRIPES:
            return tile_center_horizontal(pos * m, pt.arg0.x) * im;

        default:
            return pos;
    }
}

#endif // _TAV_SPATIAL_GLSL_

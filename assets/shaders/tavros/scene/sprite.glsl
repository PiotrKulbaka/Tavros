#ifndef _TAV_SPRITE_GLSL_
#define _TAV_SPRITE_GLSL_

#define HAS_SPRITE 0x01

struct sprite_t // 4 + 4 + 24  + 8 + 8 = 48 bytes
{
    uint   flags;
    uint   reserved;
    vec2   center;
    vec2   x_axis; // half-extent along local X
    vec2   y_axis; // half-extent along local Y
    vec2   src_xy; // source sprite xy
    vec2   src_wh; // source sprite wh
};

vec2 sample_repeat(vec2 uv, vec2 rect_xy, vec2 rect_wh)
{
    vec2 tiled_uv = mod(uv, 1.0);
    vec2 tile_offset = floor(uv);
    return vec2(tile_offset + rect_xy + tiled_uv * rect_wh);
}

vec4 sample_sprite(const in sprite_t spr, const in sampler2D tex, const in vec2 uv)
{
    if ((spr.flags & HAS_SPRITE) == HAS_SPRITE) {
        return texture(tex, uv);
    }

    return vec4(1.0);
}

#endif // _TAV_SPRITE_GLSL_

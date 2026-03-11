#ifndef TAVROS_COLOR_GLSL
#define TAVROS_COLOR_GLSL

vec4 unpack_color(uint cl)
{
    return vec4(
        float((cl >> 24) & 0xFFu) / 255.0,
        float((cl >> 16) & 0xFFu) / 255.0,
        float((cl >>  8) & 0xFFu) / 255.0,
        float((cl >>  0) & 0xFFu) / 255.0
    );
}

#endif // TAVROS_COLOR_GLSL

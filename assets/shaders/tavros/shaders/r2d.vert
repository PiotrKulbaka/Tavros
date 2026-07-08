#include <tavros/scene/scene.glsl>

#define SHAPE_TYPE_ONE        0
#define SHAPE_TYPE_ZERO       1
#define SHAPE_TYPE_ROUND_RECT 2
#define SHAPE_TYPE_CIRCLE     3
#define SHAPE_TYPE_LINE       4

struct shape_t
{
    uint  type;  // shape type, one of: SHAPE_TYPE_ONE (full geometry fill), SHAPE_TYPE_ZERO (for no cut shape), SHAPE_TYPE_ROUND_RECT, SHAPE_TYPE_CIRCLE, SHAPE_TYPE_LINE
    float rot;   // shape rotation, only for round rect
    vec2  pos0;  // center of circle, center of round rect, first point of the line
    vec2  pos1;  // round rect half sizes, pos1.x - circle radius, second point of the line
    vec4  round; // round rect radius
};


#define SPRITE_TYPE_NONE                0 // No texture - maybe better color with alpha
#define SPRITE_TYPE_ENTRY               1 // The whole texture
#define SPRITE_TYPE_RGN_CLAMP_ZERO      2 // Texture region with clamp to zero alpha
#define SPRITE_TYPE_RGN_REPEAT          3 // Texture region with repeating
#define SPRITE_TYPE_RGN_MIRRORED_REPEAT 4 // Texture region with mirrored repeating

struct sprite_t
{
    uint  type;       // One of SPRITE_TYPE_NONE, SPRITE_TYPE_ENTRY
    float rot;        // Texture rotation
    uvec2 bounds;     // Bounds rect
    vec2  center;     // center of the texture
    vec2  half_sizes; // texture draw half sizes
}

layout(PUSH_CONSTANT) uniform PushConstants
{
    brush_t  brush;
    shape_t  fill_shape;
    shape_t  cut_shape;
    sprite_t sprite;
} pc;



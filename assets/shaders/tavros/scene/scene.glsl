#ifndef TAVROSCENE_GLSL
#define TAVROSCENE_GLSL

#ifdef TAV_OPEN_GL
// 15 - k_constant_buffer_index in command_queue_opengl.cpp
#define PUSH_CONSTANT std140, binding = 15
#elif TAV_VULKAN
#define PUSH_CONSTANT push_constant
#else
#error "backend not defined"
#endif

// layout(std430, binding = N) buffer Name - only for SSBO
// layout(std140, binding = N) uniform Name - only for UBO (vec3 padding rule)

layout (std140, binding = 0) uniform Scene
{
    // ---------------------------------------------------------------
    // Matrices
    // ---------------------------------------------------------------
    mat4 view;
    mat4 projection;          // perspective, main camera
    mat4 view_projection;
    mat4 inverse_view;
    mat4 inverse_projection;
    mat4 ortho_projection;    // screen-space, origin top-left

    // ---------------------------------------------------------------
    // Camera
    // ---------------------------------------------------------------
    vec3  camera_position;    // world space
    float _pad0;
    float near_plane;
    float far_plane;
    float aspect_ratio;       // width / height
    float fov_y;              // radians
    float _pad1;

    // ---------------------------------------------------------------
    // Frame
    // ---------------------------------------------------------------
    vec2  frame_size;         // (width, height) in pixels
    vec2  frame_size_inv;     // (1/width, 1/height), avoids per-shader division

    // ---------------------------------------------------------------
    // Time
    // ---------------------------------------------------------------
    float time;         // seconds since app start
    float frame_time;   // seconds since last frame
    uint  frame_index;  // monotonically increasing, wraps at ~4B
    float _pad2;
} scene;

#define BRUSH_SOLID           0
#define BRUSH_LINEAR_GRADIENT 1
#define BRUSH_RADIAL_GRADIENT 2

layout (std140, binding = 1) uniform Brush
{
    vec2  pos0;    // solid: ignored | linear: start point | radial: center
    vec2  pos1;    // solid: ignored | linear: end point   | radial: x - radius
    vec4  color0;
    vec4  color1;
    int   type;
} brush;

vec4 sample_brush(vec2 p)
{
    float t = 0.0;

    if (brush.type == BRUSH_SOLID) {
        return brush.color0;
    } else if (brush.type == BRUSH_LINEAR_GRADIENT) {
        vec2 dir = brush.pos1 - brush.pos0;
        t = dot(p - brush.pos0, dir) / max(dot(dir, dir), 1e-6);
    } else if (brush.type == BRUSH_RADIAL_GRADIENT) {
        t = length(p - brush.pos0) / max(brush.pos1.x, 1e-6);
    }

    t = clamp(t, 0.0, 1.0);
    return mix(brush.color0, brush.color1, t);
}

const vec2 k_quad[6] = vec2[6]
(
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

#endif // TAVROSCENE_GLSL

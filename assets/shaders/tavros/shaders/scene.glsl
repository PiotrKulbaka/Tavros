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

layout (std430, binding = 0) buffer  Scene
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
    float near_plane;
    float far_plane;
    float aspect_ratio;       // width / height
    float fov_y;              // radians
    float _pad0;

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
    float _pad1;
} scene;

#endif // TAVROSCENE_GLSL

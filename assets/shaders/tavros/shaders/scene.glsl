#ifndef TAVROS_SCENE_GLSL
#define TAVROS_SCENE_GLSL

layout (std430, binding = 0) buffer  Scene
{
    // ---------------------------------------------------------------
    // Matrices
    // ---------------------------------------------------------------
    mat4 s_view;
    mat4 s_projection;          // perspective, main camera
    mat4 s_view_projection;
    mat4 s_inverse_view;
    mat4 s_inverse_projection;
    mat4 s_ortho_projection;    // screen-space, origin top-left

    // ---------------------------------------------------------------
    // Camera
    // ---------------------------------------------------------------
    vec3  s_camera_position;    // world space
    float s_near_plane;
    float s_far_plane;
    float s_aspect_ratio;       // width / height
    float s_fov_y;              // radians
    float _s_pad0;

    // ---------------------------------------------------------------
    // Frame
    // ---------------------------------------------------------------
    vec2  s_frame_size;         // (width, height) in pixels
    vec2  s_frame_size_inv;     // (1/width, 1/height), avoids per-shader division

    // ---------------------------------------------------------------
    // Time
    // ---------------------------------------------------------------
    float s_time;         // seconds since app start
    float s_delta_time;   // seconds since last frame
    uint  s_frame_index;  // monotonically increasing, wraps at ~4B
    float _s_pad1;
};

#endif // TAVROS_SCENE_GLSL

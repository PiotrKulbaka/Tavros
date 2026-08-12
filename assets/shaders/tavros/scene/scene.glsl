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
    // 3D scene
    // ---------------------------------------------------------------
    mat4 view;
    mat4 projection;          // perspective, main camera
    mat4 view_projection;
    mat4 inverse_view;
    mat4 inverse_projection;

    // ---------------------------------------------------------------
    // 3D camera
    // ---------------------------------------------------------------
    vec3  camera_position;    // world space
    float near_plane;
    float far_plane;
    float aspect_ratio;       // width / height
    float fov_y;              // radians
    float _pad0;

    // ---------------------------------------------------------------
    // UI scene
    // ---------------------------------------------------------------
    mat4 ui_projection;       // orthographic, pixel-space

    // ---------------------------------------------------------------
    // Frame
    // ---------------------------------------------------------------
    vec2  frame_size;         // (width, height) in pixels
    vec2  frame_size_inv;     // (1/width, 1/height)

    // ---------------------------------------------------------------
    // Time
    // ---------------------------------------------------------------
    float total_time;
    float delta_time; // current frame time
    uint  frame_number;
    float _pad1;
} scene;



// ---------------------------------------------------------------
// 3D coordinate space conversion helpers.
// All functions operate on the active 3D camera stored in `scene`.
// ---------------------------------------------------------------

// Converts a world-space position to clip space.
vec4 world_to_clip(vec3 world_pos)
{
    return scene.view_projection * vec4(world_pos, 1.0);
}

// Converts a homogeneous world-space position to clip space.
vec4 world_to_clip(vec4 world_pos)
{
    return scene.view_projection * world_pos;
}

// Converts a world-space position to view (camera) space.
vec3 world_to_view(vec3 world_pos)
{
    return (scene.view * vec4(world_pos, 1.0)).xyz;
}

// Converts a view-space position to world space.
vec3 view_to_world(vec3 view_pos)
{
    return (scene.inverse_view * vec4(view_pos, 1.0)).xyz;
}

// Returns a non-normalized vector from the camera position
// to the specified world-space position.
vec3 camera_to_world_vector(vec3 world_pos)
{
    return world_pos - scene.camera_position;
}

// Returns the linear distance from the camera
// to the specified world-space position.
float world_distance_from_camera(vec3 world_pos)
{
    return length(world_pos - scene.camera_position);
}

// ---------------------------------------------------------------
// Depth reconstruction helpers.
// ---------------------------------------------------------------

// Reconstructs a world-space position from screen UV coordinates
// and a depth buffer value in the [0..1] range.
vec3 depth_to_world(vec2 uv, float depth)
{
    vec4 clip_pos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    vec4 view_pos = scene.inverse_projection * clip_pos;
    view_pos /= view_pos.w;

    vec4 world_pos = scene.inverse_view * view_pos;
    return world_pos.xyz;
}

// Converts a non-linear depth buffer value into
// linear eye-space depth.
float depth_to_linear(float depth)
{
    float z = depth * 2.0 - 1.0;

    return
        (2.0 * scene.near_plane * scene.far_plane) /
        (scene.far_plane + scene.near_plane -
        z * (scene.far_plane - scene.near_plane));
}

// ---------------------------------------------------------------
// UI coordinate space conversion helpers.
// Pixel coordinates assume a top-left origin.
// ---------------------------------------------------------------

// Converts a UI pixel-space position to clip space.
vec4 ui_to_clip(vec2 pixel_pos)
{
    return scene.ui_projection * vec4(pixel_pos, 0.0, 1.0);
}

// Converts a pixel-space position to normalized UV coordinates.
vec2 ui_to_uv(vec2 pixel_pos)
{
    return pixel_pos * scene.frame_size_inv;
}

// Converts normalized UV coordinates to pixel-space.
vec2 uv_to_ui(vec2 uv)
{
    return uv * scene.frame_size;
}

// Converts a pixel-space position to normalized device coordinates.
vec2 ui_to_ndc(vec2 pixel_pos)
{
    vec2 uv = ui_to_uv(pixel_pos);

    return vec2(
        uv.x * 2.0 - 1.0,
        1.0 - uv.y * 2.0);
}

// ---------------------------------------------------------------
// Frame and timing helpers.
// ---------------------------------------------------------------

// Converts UV coordinates into aspect-corrected UV coordinates.
// Useful for circular and radial fullscreen effects.
vec2 uv_to_aspect_corrected(vec2 uv)
{
    vec2 centered = uv - 0.5;
    centered.x *= scene.aspect_ratio;
    return centered + 0.5;
}

// Returns a repeating pulse in the [0..1] range.
// `speed` is specified in cycles per second.
float time_to_pulse(float speed)
{
    return 0.5 + 0.5 * sin(scene.total_time * speed * 6.28318530718);
}

// Computes a framerate-independent interpolation factor.
// Usage:
// value = mix(value, target, exponential_lerp_factor(rate));
float exponential_lerp_factor(float rate)
{
    return 1.0 - exp(-rate * scene.delta_time);
}



// Converts clip-space coordinates to normalized UV coordinates.
vec2 clip_to_uv(vec4 clip_pos)
{
    vec2 ndc = clip_pos.xy / clip_pos.w;
    return ndc * 0.5 + 0.5;
}

// Converts normalized UV coordinates to NDC.
vec2 uv_to_ndc(vec2 uv)
{
    return vec2(
        uv.x * 2.0 - 1.0,
        uv.y * 2.0 - 1.0);
}

// Converts NDC coordinates to normalized UV coordinates.
vec2 ndc_to_uv(vec2 ndc)
{
    return ndc * 0.5 + 0.5;
}

// Returns a normalized direction from the camera
// to a world-space position.
vec3 camera_to_world_direction(vec3 world_pos)
{
    return normalize(
        world_pos - scene.camera_position);
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

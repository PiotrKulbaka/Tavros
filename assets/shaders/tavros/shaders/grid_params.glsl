#ifndef TAVROS_GRID_PARAMS_GLSL
#define TAVROS_GRID_PARAMS_GLSL

layout(std430, binding = 1) buffer GridParams
{
    // Plane
    vec3  g_plane_normal;
    float g_plane_offset;
    vec3  g_plane_origin;
    float _pad0;

    // Grid
    float g_minor_step;
    float g_major_step;
    float g_fade_distance;
    float g_line_width_scale;

    // colors
    vec4 g_x_axis_color;
    vec4 g_y_axis_color;
    vec4 g_minor_color;
    vec4 g_major_color;
};

#endif // TAVROS_GRID_PARAMS_GLSL

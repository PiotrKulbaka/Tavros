layout (std430, binding = 0) buffer Scene
{
    mat4 u_view;
    mat4 u_perspective_projection;
    mat4 u_view_projection;
    mat4 u_inverse_view;
    mat4 u_inverse_projection;
    mat4 u_ortho;

    float u_frame_width;
    float u_frame_height;

    float u_near_plane;
    float u_far_plane;
    float u_view_space_depth;
    float u_aspect_ratio;
    float u_fov_y;
};

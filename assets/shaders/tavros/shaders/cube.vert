layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

layout (std430, binding = 0) buffer Scene
{
    mat4 u_view;
    mat4 u_perspective_projection;
    mat4 u_view_perspective_projection;
    mat4 u_inverse_view;
    mat4 u_inverse_perspective_projection;
    mat4 u_ortho;

    float u_frame_width;
    float u_frame_height;

    float u_near_plane;
    float u_far_plane;
    float u_view_space_depth;
    float u_aspect_ratio;
    float u_fov_y;
};

out vec2 v_tex_coord;
out vec3 v_normal;
out vec3 v_to_camera;

void main()
{
    vec4 world_pos = vec4(a_pos, 1.0); // u_model * vec4(a_pos, 1.0);

    v_normal = a_normal; // mat3(u_model) * a_normal;

    vec3 camera_pos = (u_inverse_view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    v_to_camera = normalize(camera_pos - world_pos.xyz);

    v_tex_coord = a_uv;

    gl_Position = u_view_perspective_projection * vec4(a_pos, 1.0);
}

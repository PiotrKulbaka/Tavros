#include <tavros/shaders/plane_quad.glsl>
#include <tavros/shaders/scene.glsl>

out vec3 v_world_pos;
out vec3 v_cam_pos;
out float v_minor_grid_step;
out float v_major_grid_step;
out float v_line_width;
out float v_view_space_depth;
out float v_near;
out float v_far;

void main()
{
    v_near = u_near_plane;
    v_far = u_far_plane;

    const float SQRT2 = 1.4142135623730951;

    float plane_scale = (u_view_space_depth) * SQRT2;
    v_cam_pos = (u_inverse_view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;

    float minor_step_base = 1.0;
    float major_step_base = 10.0;

    v_view_space_depth = u_view_space_depth;

    // thresholds
    float t1 = u_view_space_depth * 0.03;
    float t2 = u_view_space_depth * 0.3;

    float cam_height = abs(v_cam_pos.z);
    float mask1 = 1.0 - step(t1, cam_height);
    float mask10 = step(t1, cam_height) * (1.0 - step(t2, cam_height));
    float mask100 = step(t2, cam_height); 

    float step1 = 1.0;
    float step10 = 10.0;
    float step100 = 100.0;

    float scale_factor = step1 * mask1 + step10 * mask10 + step100 * mask100;
    v_minor_grid_step = minor_step_base * scale_factor;
    v_major_grid_step = major_step_base * scale_factor;
    
    v_line_width = 0.01 * pow(cam_height, 0.63);

    vec2 pos = quadVerts[gl_VertexID];
    v_world_pos = vec3(pos.x * plane_scale + v_cam_pos.x, pos.y * plane_scale + v_cam_pos.y, 0.0);
    gl_Position = u_view_projection * vec4(v_world_pos, 1.0);
}

in vec3 v_world_pos;
in vec3 v_cam_pos;
in float v_minor_grid_step;
in float v_major_grid_step;
in float v_line_width;
in float v_view_space_depth;
in float v_near;
in float v_far;

layout(location = 0) out vec4 out_color;

void main()
{
    const vec3 x_axis_color = vec3(0.3, 1.0, 0.3);
    const vec3 y_axis_color = vec3(1.0, 0.3, 0.3);
    const vec3 minor_grid_color = vec3(0.3);
    const vec3 major_grid_color = vec3(0.4);

    vec2 world_xy = abs(v_world_pos.xy);

    // axis lines
    vec2 axis_mask = step(world_xy, vec2(v_line_width * 2.0));
    float xy_mask = max(axis_mask.x, axis_mask.y);

    // major lines
    vec2 major_lines = step(mod(world_xy, v_major_grid_step), vec2(v_line_width * 2.0));
    float major_mask = clamp(max(major_lines.x, major_lines.y) - xy_mask, 0.0, 1.0);

    // minor lines
    vec2 minor_lines = step(mod(world_xy, v_minor_grid_step), vec2(v_line_width));
    float minor_mask = clamp(max(minor_lines.x, minor_lines.y) - major_mask, 0.0, 1.0);
    
    float final_mask = xy_mask + major_mask + minor_mask;
    
    vec3 final_color = x_axis_color * axis_mask.x + y_axis_color * axis_mask.y + minor_grid_color * minor_mask + major_grid_color * major_mask;

    // fading by camera angle
    vec3 view_dir = normalize(v_cam_pos - v_world_pos);
    float angle_factor = clamp(abs(view_dir.z) * 2.2, 0.0, 1.0);

    // fading by distance
    float dist_factor = clamp(
        1.0
        - smoothstep(v_view_space_depth * 0.85, v_view_space_depth * 0.95, length(v_cam_pos.xy - v_world_pos.xy))
        - smoothstep(v_view_space_depth * 0.45, v_view_space_depth * 0.5, v_cam_pos.z)
        , 0.0, 1.0
    );

    float final_alpha = dist_factor *  angle_factor * final_mask * 0.8;

    out_color = vec4(final_color, final_alpha);
}

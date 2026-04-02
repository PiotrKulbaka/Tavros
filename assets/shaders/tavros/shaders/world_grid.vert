#include <tavros/shaders/scene.glsl>

// ----------------------------------------------------------------
// Inputs
// ----------------------------------------------------------------
layout(PUSH_CONSTANT) uniform PushConstants
{
    uint  minor_color;
    uint  major_color;
    uint  axis_u_color;      // color of the first axis  (X for XY/ZX, Y for YZ)
    uint  axis_v_color;      // color of the second axis (Y for XY, Z for YZ/ZX)
    float minor_line_width;  // pixels
    float major_line_width;  // pixels
    float axis_line_width;   // pixels
    float minor_grid_step;   // world units
    float major_grid_step;   // world units
    uint  flags;             // bits [1:0] — plane: 0 = XY, 1 = YZ, 2 = ZX
} pc;

// ----------------------------------------------------------------
// Outputs
// ----------------------------------------------------------------
out vec2  v_uv;
out vec3  v_cam_pos;
out vec3  v_plane_normal;   // world-space normal of the grid plane
out vec3  v_world_pos;
out float v_view_space_depth;

// ----------------------------------------------------------------

const vec2 k_quad_verts[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

void main()
{
    float view_space_depth = scene.far_plane - scene.near_plane;
    float plane_scale      = view_space_depth * 2.0;

    v_cam_pos          = (scene.inverse_view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    v_view_space_depth = view_space_depth;

    uint plane_id = pc.flags & 0x3u;

    // Each plane: quad is centered at camera projected onto that plane,
    // normal is the axis perpendicular to it.
    vec2 pos = k_quad_verts[gl_VertexID] * plane_scale;
    if (plane_id == 0u) { // XY plane (Z = 0) — normal is Z
        v_plane_normal = vec3(0.0, 0.0, 1.0);
        v_world_pos    = vec3(v_cam_pos.xy, 0.0) + vec3(pos, 0.0);
        v_uv           = v_world_pos.xy;
    } else if (plane_id == 1u) { // YZ plane (X = 0) — normal is X
        v_plane_normal = vec3(1.0, 0.0, 0.0);
        v_world_pos    = vec3(0.0, v_cam_pos.yz) + vec3(0.0, pos);
        v_uv           = v_world_pos.yz;
    } else { // ZX plane (Y = 0) — normal is Y
        v_plane_normal = vec3(0.0, 1.0, 0.0);
        v_world_pos    = vec3(v_cam_pos.x, 0.0, v_cam_pos.z) + vec3(pos.x, 0.0, pos.y);
        v_uv           = v_world_pos.zx;
    }

    gl_Position = scene.view_projection * vec4(v_world_pos, 1.0);
}

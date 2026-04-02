#include <tavros/shaders/scene.glsl>
#include <tavros/shaders/color.glsl>

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

in vec2  v_uv;
in vec3  v_cam_pos;
in vec3  v_plane_normal;
in vec3  v_world_pos;
in float v_view_space_depth;

layout(location = 0) out vec4 out_color;

// ----------------------------------------------------------------
// Grid helpers
// ----------------------------------------------------------------

// Coverage of a repeating grid line at intervals of `step`.
// Width is specified in screen pixels; uses L2 derivative to avoid
// the line thickening at 45-degree angles that fwidth() causes.
float grid_line(float coord, float step, float line_width_px)
{
    float deriv      = length(vec2(dFdx(coord), dFdy(coord)));
    float half_width = line_width_px * 0.5 * deriv;
    float dist       = mod(abs(coord), step);
    dist             = min(dist, step - dist);
    return 1.0 - smoothstep(half_width - deriv, half_width + deriv, dist);
}

// Coverage of a single axis line sitting at perp_coord == 0.
float axis_line(float perp_coord, float line_width_px)
{
    float deriv      = length(vec2(dFdx(perp_coord), dFdy(perp_coord)));
    float half_width = line_width_px * 0.5 * deriv;
    return 1.0 - smoothstep(half_width - deriv, half_width + deriv, abs(perp_coord));
}

// ----------------------------------------------------------------
// Fade helpers
// ----------------------------------------------------------------
float angle_fade(vec3 cam_pos, vec3 world_pos, vec3 plane_normal)
{
    vec3 view_dir = normalize(cam_pos - world_pos);
    return clamp(abs(dot(view_dir, plane_normal)) * 2.2, 0.0, 1.0);
}

float distance_fade(vec3 cam_pos, vec3 world_pos, vec3 plane_normal, float view_space_depth)
{
    // Lateral distance — project fragment-to-camera vector onto the plane
    vec3  to_frag = world_pos - cam_pos;
    float lateral = length(to_frag - plane_normal * dot(to_frag, plane_normal));
    float cam_height = abs(dot(cam_pos, plane_normal));

    return clamp(
        1.0
        - smoothstep(view_space_depth * 0.85, view_space_depth * 0.99, lateral)
        - smoothstep(view_space_depth * 0.45, view_space_depth * 0.50, cam_height),
        0.0, 1.0
    );
}

vec4 blend(vec4 dst, vec4 src, float mask)
{
    vec4 s = src;
    s.a *= mask;
    float out_a = s.a + dst.a * (1.0 - s.a);
    vec3  out_rgb = out_a < 0.0001 ? vec3(0.0) : (s.rgb * s.a + dst.rgb * dst.a * (1.0 - s.a)) / out_a;
    return vec4(out_rgb, out_a);
}

void main()
{
    float axis_u  = axis_line(v_uv.y, pc.axis_line_width);
    float axis_v  = axis_line(v_uv.x, pc.axis_line_width);
    float major_u = grid_line(v_uv.y, pc.major_grid_step, pc.major_line_width);
    float major_v = grid_line(v_uv.x, pc.major_grid_step, pc.major_line_width);
    float minor_u = grid_line(v_uv.y, pc.minor_grid_step, pc.minor_line_width);
    float minor_v = grid_line(v_uv.x, pc.minor_grid_step, pc.minor_line_width);

    float axis_mask  = clamp(max(axis_u, axis_v), 0.0, 1.0);
    float major_mask = clamp(max(major_u, major_v) - axis_mask,  0.0, 1.0);
    float minor_mask = clamp(max(minor_u, minor_v) - axis_mask - major_mask, 0.0, 1.0);

    vec4 color = vec4(0.0f);
    color = blend(color, unpack_color(pc.minor_color), minor_mask);
    color = blend(color, unpack_color(pc.major_color), major_mask);
    color = blend(color, unpack_color(pc.axis_v_color), axis_v * axis_mask);
    color = blend(color, unpack_color(pc.axis_u_color), axis_u * axis_mask);

    float a_fade = angle_fade(v_cam_pos, v_world_pos, v_plane_normal); //clamp(abs(view_dir.z) * 2.2, 0.0, 1.0);
    float d_fade = distance_fade(v_cam_pos, v_world_pos, v_plane_normal, v_view_space_depth);
    color.a *= a_fade * d_fade;
    out_color = color;
}

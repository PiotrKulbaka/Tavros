#include <tavros/shaders/scene.glsl>
#include <tavros/shaders/color.glsl>
#include <tavros/shaders/plane_quad.glsl>

layout(location = 0) in vec3  a_pos;
layout(location = 1) in float a_size;
layout(location = 2) in uint  a_color;
layout(location = 3) in float a_rotation;

out vec2 v_uv;
out vec4 v_color;

void main()
{
    vec2 corner = k_quad_verts[gl_VertexID];

    float c = cos(a_rotation);
    float s = sin(a_rotation);
    vec2 rotated = vec2(corner.x * c - corner.y * s, corner.x * s + corner.y * c) * a_size;

    // Billboarding
    vec4 view_center = scene.view * vec4(a_pos, 1.0);
    view_center.xy += rotated;
    gl_Position = scene.projection * view_center;

    v_uv = k_quad_uvs[gl_VertexID];
    v_color = unpack_color(a_color);
}

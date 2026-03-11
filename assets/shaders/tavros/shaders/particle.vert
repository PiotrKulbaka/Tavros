#include <tavros/shaders/scene.glsl>
#include <tavros/shaders/color.glsl>

layout(location = 0) in vec3  a_pos;
layout(location = 1) in float a_size;
layout(location = 2) in uint  a_color;
layout(location = 3) in float a_rotation;

out vec2 v_uv;
out vec4 v_color;

const vec2 k_quad[4] = vec2[4](
    vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(-0.5, 0.5), vec2(0.5, 0.5)
);
const vec2 k_uv[4] = vec2[4](
    vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0)
);

void main()
{
    vec2 corner = k_quad[gl_VertexID];

    float c = cos(a_rotation);
    float s = sin(a_rotation);
    vec2 rotated = vec2(corner.x * c - corner.y * s,
                        corner.x * s + corner.y * c) * a_size;

    // Billboard: переводим центр в view space, смещаем по XY
    vec4 view_center = view * vec4(a_pos, 1.0); // vec3 → vec4!
    view_center.xy  += rotated;
    gl_Position      = projection * view_center;

    v_uv = k_uv[gl_VertexID];
    v_color = unpack_color(a_color);
}

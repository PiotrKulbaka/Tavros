#include <tavros/shaders/scene.glsl>

const vec2 k_quad[6] = vec2[6](
    vec2(-1, -1), vec2( 1, -1), vec2( 1,  1),
    vec2(-1, -1), vec2( 1,  1), vec2(-1,  1)
);

const vec2 k_uv[6] = vec2[6](
    vec2(0, 0), vec2(1, 0), vec2(1, 1),
    vec2(0, 0), vec2(1, 1), vec2(0, 1)
);

layout(PUSH_CONSTANT) uniform PushConstants
{
    vec3  position;
    float _pad0;
    vec2  size;
    vec2  _pad1;
    vec4  color;
    float layer;
    float rotation;
} pc;

out vec2 v_uv;
out vec4 v_color;
out float v_layer;

void main()
{
    vec2 local = k_quad[gl_VertexID];

    float c = cos(pc.rotation);
    float s = sin(pc.rotation);
    vec2 rotated = vec2(local.x * c - local.y * s, local.x * s + local.y * c) * pc.size;

    // Billboarding
    vec4 view_center = scene.view * vec4(pc.position, 1.0);
    view_center.xy += rotated;
    gl_Position = scene.projection * view_center;

    v_uv = k_uv[gl_VertexID];
    v_color = pc.color;
    v_layer = pc.layer;
}
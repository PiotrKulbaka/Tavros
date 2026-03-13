#include <tavros/shaders/plane_quad.glsl>

out vec2 v_uv;

void main()
{
    vec2 pos = k_quad_verts[gl_VertexID] * 2.0f;
    v_uv = k_quad_uvs[gl_VertexID];
    gl_Position = vec4(pos, 0.0, 1.0);
}

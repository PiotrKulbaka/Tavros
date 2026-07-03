const vec2 k_quad_verts[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

const vec2 k_quad_uvs[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

out vec2 v_uv;

void main()
{
    vec2 pos = k_quad_verts[gl_VertexID];
    v_uv = k_quad_uvs[gl_VertexID];
    gl_Position = vec4(pos, 0.0, 1.0);
}

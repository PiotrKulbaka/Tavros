#ifndef TAVROS_PLANE_QUAD_GLSL
#define TAVROS_PLANE_QUAD_GLSL

const vec2 k_quad_verts[4] = vec2[](
    vec2(-0.5, -0.5),
    vec2( 0.5, -0.5),
    vec2(-0.5,  0.5),
    vec2( 0.5,  0.5)
);

const vec2 k_quad_uvs[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

#endif // TAVROS_PLANE_QUAD_GLSL

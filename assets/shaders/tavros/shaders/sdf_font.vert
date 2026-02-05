#include <tavros/shaders/scene.glsl>

layout (location = 0) in mat3x2 a_gpyph_transform;  // per-instance transform
layout (location = 3) in uvec2 a_bounds; // per-instance glyph uvs
layout (location = 4) in uint a_gpyph_color;  // per-instance color
layout (location = 5) in uint a_outline_color;  // per-instance color

const vec2 plane_verts[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

layout(binding = 0) uniform sampler2D u_sdf_atlas;

out vec2 v_uv;
out vec4 v_color;
out vec4 v_outline_color;

vec4 unpack_color(uint cl)
{
    return vec4(
        float((cl >> 24) & 0xFFu) / 255.0,
        float((cl >> 16) & 0xFFu) / 255.0,
        float((cl >>  8) & 0xFFu) / 255.0,
        float((cl >>  0) & 0xFFu) / 255.0
    );
}

void main()
{
    int vid = gl_VertexID % 4;
    vec2 local_pos = plane_verts[vid];

    vec2 tex_size = textureSize(u_sdf_atlas, 0);
    vec4 uv0uv1 = vec4(
        float((a_bounds.x >> 0) & 0xffffu) / tex_size.x,
        float((a_bounds.x >> 16 ) & 0xffffu) / tex_size.y,
        float((a_bounds.y >> 0) & 0xffffu) / tex_size.x,
        float((a_bounds.y >> 16) & 0xffffu) / tex_size.y
    );

    // Interpolate UVs (simple quad mapping)
    v_uv = mix(uv0uv1.xy, uv0uv1.zw, local_pos);

    // Transform to clip space
    vec2 world_pos = a_gpyph_transform * vec3(local_pos, 1.0);
    gl_Position = u_ortho * vec4(world_pos, 0.0, 1.0);

    v_color = unpack_color(a_gpyph_color);
    v_outline_color = unpack_color(a_outline_color);
}

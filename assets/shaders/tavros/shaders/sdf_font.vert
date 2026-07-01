#include <tavros/scene/scene.glsl>
#include <tavros/shaders/color.glsl>

layout (location = 0) in mat3x2 a_gpyph_transform;  // per-instance transform
layout (location = 3) in uvec4  a_bounds_and_color; // per-instance xy - glyph rects; z - glyph color; w - outline color


layout(PUSH_CONSTANT) uniform PushConstants
{
    vec2 pos;
    uint flags; // fill_threshold : 8, outline_threshold : 8, use_fill_brush_mask : 1, use_outline_brush_mask : 1
} pc;

layout(binding = 0) uniform sampler2D u_atlas;

out vec2 v_uv;
out vec4 v_color;
out vec4 v_outline_color;
out vec2 v_world_pos;
out float v_fill_th;
out float v_outline_th;
flat out uint  v_flags;

void main()
{
    vec2 local_pos = (k_quad[gl_VertexID] + 1.0f) * 0.5f;

    vec2 tex_size = textureSize(u_atlas, 0);
    uvec2 bounds = a_bounds_and_color.xy;
    vec4 uv0uv1 = vec4(
        float((bounds.x >> 0) & 0xffffu) / tex_size.x,
        float((bounds.x >> 16 ) & 0xffffu) / tex_size.y,
        float((bounds.y >> 0) & 0xffffu) / tex_size.x,
        float((bounds.y >> 16) & 0xffffu) / tex_size.y
    );

    // Interpolate UVs (simple quad mapping)
    v_uv = mix(uv0uv1.xy, uv0uv1.zw, local_pos);

    // Transform to clip space
    vec2 world_pos = a_gpyph_transform * vec3(local_pos, 1.0) + pc.pos;
    v_world_pos = world_pos;
    gl_Position = scene.ortho_projection * vec4(world_pos, 0.0, 1.0);

    v_color = unpack_color(a_bounds_and_color.z);
    v_outline_color = unpack_color(a_bounds_and_color.w);

    uint fill_th = (pc.flags >> 0) & 0xff;
    uint outline_th = (pc.flags >> 8) & 0xff;

    v_fill_th = float(fill_th) / 255.0f;
    v_outline_th = v_fill_th - float(outline_th) / 255.0f;
    v_flags = uint((pc.flags >> 16) & 0x3) | (uint(outline_th != 0) << 2);
}

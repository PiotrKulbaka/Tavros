#include <tavros/scene/scene.glsl>

layout (location = 0) in vec4 a_pos;
layout (location = 1) in vec3 a_norm;
layout (location = 2) in uint a_color;
layout (location = 3) in vec2 a_uv;

out vec3 v_world_pos;
out vec3 v_normal;
out vec4 v_color;
out vec2 v_uv;

void main()
{
    // Paste something here: view_proj matrix name from the Scene block (scene.glsl)
    gl_Position = world_to_clip(a_pos.xyz);

    // Works only if GL_PROGRAM_POINT_SIZE is enabled in the pipeline (points materials)
    gl_PointSize = max(a_pos.w, 1.0);

    v_world_pos = a_pos.xyz;
    v_normal = a_norm;
    v_color = unpackUnorm4x8(a_color); // assumes r in the lowest byte of color32::packed
    v_uv = a_uv;
}

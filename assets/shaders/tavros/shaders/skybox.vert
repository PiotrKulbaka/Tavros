#include <tavros/shaders/scene.glsl>

const vec3 k_positions[36] = vec3[36](
    // +X face (forward)
    vec3(1, 1, 1), vec3(1, 1, -1),  vec3(1, -1, -1),
    vec3(1, 1, 1), vec3(1, -1,  -1), vec3(1, -1, 1),
    // -X face (backward)
    vec3(-1,  1, -1), vec3(-1, -1, -1), vec3(-1, -1,  1),
    vec3(-1,  1, -1), vec3(-1, -1,  1), vec3(-1,  1,  1),
    // +Y face (right)
    vec3(-1,  1, -1), vec3(-1,  1,  1), vec3( 1,  1,  1),
    vec3(-1,  1, -1), vec3( 1,  1,  1), vec3( 1,  1, -1),
    // -Y face (left)
    vec3( 1, -1, -1), vec3( 1, -1,  1), vec3(-1, -1,  1),
    vec3( 1, -1, -1), vec3(-1, -1,  1), vec3(-1, -1, -1),
    // +Z face (up)
    vec3(-1, -1,  1), vec3( 1, -1,  1), vec3( 1,  1,  1),
    vec3(-1, -1,  1), vec3( 1,  1,  1), vec3(-1,  1,  1),
    // -Z face (down)
    vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1, -1, -1), vec3(-1,  1, -1), vec3( 1,  1, -1)
);

out vec3 v_dir;

void main()
{
    vec3 pos = k_positions[gl_VertexID];
    v_dir = pos;

    mat4 view_only = mat4(mat3(scene.view));
    vec4 out_pos = scene.projection * view_only * vec4(pos, 1.0);
    gl_Position = out_pos.xyww;

}

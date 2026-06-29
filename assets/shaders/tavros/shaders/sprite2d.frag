#include <tavros/scene/scene.glsl>

layout(binding = 0) uniform sampler2D u_texture;

in vec2 v_uv;
in vec2 v_coord;

out vec4 base_color;

void main()
{
    vec4 modulate = sample_brush(v_coord);
    base_color = texture(u_texture, v_uv) * modulate;
}
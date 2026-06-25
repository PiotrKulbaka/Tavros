in vec2 v_uv;

out vec4 base_color;

layout(binding = 0) uniform sampler2D u_tex;

void main()
{
    vec3 color = texture(u_tex, v_uv).rgb;
    base_color = vec4(color, 1.0f);
}


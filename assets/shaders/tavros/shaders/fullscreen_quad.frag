in vec2 v_uv;

out vec4 out_color;

layout(binding = 0) uniform sampler2D u_tex;

void main()
{
    vec3 color = texture(u_tex, v_uv).rgb;
    out_color = vec4(color, 1.0f);
}

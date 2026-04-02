layout(binding = 0) uniform sampler2DArray u_tex;

in vec2  v_uv;
in vec4  v_color;
in float v_layer;

out vec4 frag_color;

void main()
{
    vec4 tex_color = texture(u_tex, vec3(v_uv, v_layer));

    if (tex_color.a < 0.01) {
       discard;
    }

    frag_color = tex_color;
}

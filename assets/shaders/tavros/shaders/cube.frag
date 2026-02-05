layout(binding = 0) uniform sampler2D uTex;

in vec2 v_tex_coord;
in vec3 v_normal;
in vec3 v_to_camera;

layout(location = 0) out vec4 out_color;

void main()
{
    vec3 N = gl_FrontFacing ? v_normal : -v_normal;
    vec3 L = normalize(v_to_camera);

    float diffuse = max(dot(N, L), 0.0);
    float ambient = 0.25;
    float diffuse_intensity = ambient + diffuse * 1.25;
    vec3 lighting = vec3(min(diffuse_intensity, 1.0f));

    vec3 base_color = texture(uTex, v_tex_coord).rgb;

    out_color = vec4(base_color * lighting, 1.0);
}

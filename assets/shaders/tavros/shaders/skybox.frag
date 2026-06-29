in vec3 v_dir;

layout(location = 0) out vec4 base_color;

layout(binding = 0) uniform samplerCube u_skybox;

void main()
{
    vec4 cl = texture(u_skybox, v_dir);
    base_color = cl;
}

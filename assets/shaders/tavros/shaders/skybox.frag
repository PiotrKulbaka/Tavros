in vec3 v_dir;

layout(location = 0) out vec4 frag_color;

layout(binding = 0) uniform samplerCube u_skybox;

void main()
{
    frag_color = texture(u_skybox, v_dir);
}

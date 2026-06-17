in vec3 v_dir;

layout(location = 0) out vec4 frag_color;

layout(binding = 0) uniform samplerCube u_skybox;
layout(binding = 12) uniform samplerCube u_skybox2;

void main()
{
    vec4 cl = texture(u_skybox, v_dir);
    vec4 cl2 = texture(u_skybox2, v_dir);
    frag_color = cl;
}

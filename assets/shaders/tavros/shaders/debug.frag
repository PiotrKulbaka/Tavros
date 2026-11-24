#include <tavros/scene/scene.glsl>

layout(binding = 0) uniform sampler2D u_tex;

in vec3 v_world_pos;
in vec3 v_normal;
in vec4 v_color;
in vec2 v_uv;

layout(location = 0) out vec4 base_color;

float sat1(float x)
{
    return clamp(x, 0.0, 1.0);
}

void main()
{
    vec4 albedo = v_color * texture(u_tex, v_uv);

    // Juicier colors
    float luma = dot(albedo.rgb, vec3(0.2126, 0.7152, 0.0722));
    albedo.rgb = max(mix(vec3(luma), albedo.rgb, scene.debug_saturation), vec3(0.0));

    vec3 V = normalize(scene.camera_position - v_world_pos);

    // No normal (zero vector) - the surface faces the camera
    float n_len2 = dot(v_normal, v_normal);
    vec3  N = n_len2 > 1e-6 ? v_normal * inversesqrt(n_len2) : V;

    // Two-sided lighting: flip the normal towards the viewer
    N = faceforward(N, -V, N);

    vec3  L = normalize(scene.debug_sun_direction);
    vec3  H = normalize(L + V);
    float n_l = dot(N, L);
    float n_v = sat1(dot(N, V));

    // Sun: wrapped diffuse
    float wrap = scene.debug_light_wrap;
    float diffuse = sat1((n_l + wrap) / (1.0 + wrap));
    vec3  sun = scene.debug_sun_color * (scene.debug_sun_intensity * diffuse);

    // Hemisphere ambient: sky above (+Z), ground below (-Z)
    vec3 ambient = mix(scene.debug_ground_color, scene.debug_sky_color, N.z * 0.5 + 0.5)
                 * scene.debug_ambient_intensity;

    // Fill light from the camera side
    vec3 fill = scene.debug_fill_color * (scene.debug_fill_intensity * n_v);

    // Specular highlight of the sun (only on the lit side)
    float spec = pow(sat1(dot(N, H)), scene.debug_specular_power) * sat1(n_l * 4.0);
    vec3  specular = scene.debug_sun_color * (scene.debug_specular_intensity * spec);

    // Rim light at grazing angles
    float rim = pow(1.0 - n_v, scene.debug_rim_power) * scene.debug_rim_intensity;
    vec3  rim_color = scene.debug_sky_color * rim;

    vec3 color = albedo.rgb * (ambient + sun + fill) + specular + rim_color;

    base_color = vec4(color, albedo.a);
}

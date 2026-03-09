in vec2  v_uv;
in vec4  v_color;

out vec4 out_color;

// Optional particle texture (soft circle / spark / smoke sprite)
// If no texture is bound — falls back to SDF soft circle
layout(binding = 0) uniform sampler2D u_particle_tex;

// 0 = use texture, 1 = use SDF circle (set via push constant or uniform)
layout(location = 0) uniform int u_use_texture;

// Soft circle SDF — smooth edge, no hard aliasing
float soft_circle(vec2 uv, float radius, float softness)
{
    float dist = length(uv - vec2(0.5));
    return 1.0 - smoothstep(radius - softness, radius, dist);
}

void main()
{
    vec4 color = v_color;

    if (u_use_texture == 1) {
        // Textured particle (smoke, spark, fire sprite)
        vec4 tex = texture(u_particle_tex, v_uv);
        color *= tex;
    } else {
        // Procedural soft circle
        float alpha = soft_circle(v_uv, 0.45, 0.1);
        color.a *= alpha;
    }

    // Discard fully transparent fragments
    if (color.a < 0.001) {
        discard;
    }

    out_color = color;
}

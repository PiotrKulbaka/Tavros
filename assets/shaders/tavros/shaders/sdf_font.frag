out vec4 frag_color;

layout(binding = 0) uniform sampler2D u_sdf_atlas;

in vec2 v_uv;
in vec4 v_color;
in vec4 v_outline_color;

void main()
{
    float sdf = texture(u_sdf_atlas, v_uv).r;

    // Thresholds and smoothing for SDF
    float smooth_th = 0.05;
    float text_th = 0.5;
    float outline_th = 0.35;

    // Alpha masks for text and outline
    float text_alpha = smoothstep(text_th, text_th + smooth_th, sdf);
    float outline_alpha = smoothstep(outline_th, outline_th + smooth_th, sdf);

    // Color interpolation between outline and main text
    vec4 color = mix(v_outline_color, v_color, text_alpha);

    // Combine alpha channels properly
    float final_alpha = max(outline_alpha * color.a, 0.0);

    frag_color = vec4(color.rgb, final_alpha);
}

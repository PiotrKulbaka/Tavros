out vec4 frag_color;

layout(binding = 0) uniform sampler2D u_sdf_atlas;

in vec2 v_uv;
in vec4 v_color;
in vec4 v_outline_color;

void main()
{
    float sdf = texture(u_sdf_atlas, v_uv).r;

    // Compute adaptive smoothing based on screen-space derivatives
    float w = max(fwidth(sdf) * 0.5, 0.004);

    // Core thresholds (can stay constants or become uniforms)
    float text_th = 0.5;
    //float outline_th = 0.4;

    // Anti-aliased alpha
    float text_alpha = smoothstep(text_th - w, text_th + w, sdf);
    // float outline_alpha = smoothstep(outline_th - w, outline_th + w, sdf);

    // Outline only where text is not fully opaque
    // float final_outline = outline_alpha * (1.0 - text_alpha);

    //vec4 color = mix(v_outline_color, v_color, text_alpha);

    // float final_alpha = text_alpha * v_color.a + final_outline * v_outline_color.a;

    frag_color = vec4(v_color.rgb, text_alpha);
}
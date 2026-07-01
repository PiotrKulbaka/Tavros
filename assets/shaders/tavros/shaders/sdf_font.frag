#include <tavros/scene/scene.glsl>

out vec4 base_color;

layout(binding = 0) uniform sampler2D u_atlas;

in vec2  v_uv;
in vec4  v_color;
in vec4  v_outline_color;
in vec2  v_world_pos;
in float v_fill_th;
in float v_outline_th;
flat in uint  v_flags;

#define USE_FILL_BRUSH_MASK 0x1
#define USE_OUTLINE_BRUSH_MASK 0x2
#define USE_DRAW_OUTLINE 0x4

void main()
{
    float sdf = texture(u_atlas, v_uv).r;

    // Adaptive smoothing based on screen-space derivatives
    float w = max(fwidth(sdf) * 0.5, 0.004);

    // Resolve fill color
    vec4 brush = sample_brush(v_world_pos);
    vec4 fill_color = v_color;
    if ((v_flags & USE_FILL_BRUSH_MASK) != 0u) {
        fill_color *= brush;
    }

    // Fill alpha
    float fill_alpha = smoothstep(v_fill_th - w, v_fill_th + w, sdf);

    // Early out — no outline
    if ((v_flags & USE_DRAW_OUTLINE) == 0u) {
        base_color = vec4(fill_color.rgb, fill_color.a * fill_alpha);
        return;
    }

    // Resolve outline color
    vec4 outline_color = v_outline_color;
    if ((v_flags & USE_OUTLINE_BRUSH_MASK) != 0u) {
        outline_color *= brush;
    }

    // Outline alpha — visible only where fill is not fully opaque
    float outline_alpha = smoothstep(v_outline_th - w, v_outline_th + w, sdf);
    float final_outline = outline_alpha * (1.0 - fill_alpha);

    // Composite: outline under fill
    vec3 blended_rgb = mix(outline_color.rgb, fill_color.rgb, fill_alpha);
    float blended_a  = fill_color.a * fill_alpha + outline_color.a * final_outline;

    base_color = vec4(blended_rgb, blended_a);
}

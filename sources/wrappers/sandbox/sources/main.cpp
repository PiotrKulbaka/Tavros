#include <stdio.h>

#include "render_app_base.hpp"
#include "image_codec.hpp"
#include "built_in_meshes.hpp"
#include "input_manager.hpp"

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/camera/camera.hpp>
#include <tavros/renderer/render_target.hpp>

#include <tavros/resources/resource_manager.hpp>
#include <tavros/resources/providers/filesystem_provider.hpp>
#include <tavros/core/memory/buffer.hpp>

#include <tavros/system/application.hpp>

#include <algorithm>

#include <tavros/core/geometry/aabb2.hpp>

#include <tavros/ui/font/truetype_font.hpp>
#include <tavros/ui/font/font_atlas.hpp>

namespace rhi = tavros::renderer::rhi;

const char* fullscreen_quad_vertex_shader_source = R"(
#version 430 core

const vec2 quadVerts[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);
const vec2 quadUVs[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);
out vec2 texCoord;

void main()
{
    vec2 pos = quadVerts[gl_VertexID];
    texCoord = quadUVs[gl_VertexID];
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

const char* fullscreen_quad_fragment_shader_source = R"(
#version 430 core

in vec2 texCoord;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uTex;

void main()
{
    vec3 color = texture(uTex, texCoord).rgb;
    FragColor = vec4(color, 1.0f);
}
)";

const char* mesh_renderer_vertex_shader_source = R"(
#version 430 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

layout (std430, binding = 0) buffer Scene
{
    mat4 u_view;
    mat4 u_perspective_projection;
    mat4 u_view_perspective_projection;
    mat4 u_inverse_view;
    mat4 u_inverse_perspective_projection;
    mat4 u_ortho;

    float u_frame_width;
    float u_frame_height;

    float u_near_plane;
    float u_far_plane;
    float u_view_space_depth;
    float u_aspect_ratio;
    float u_fov_y;
};

out vec2 v_tex_coord;
out vec3 v_normal;
out vec3 v_to_camera;
out float v_near;
out float v_far;

void main()
{
    v_near = u_near_plane;
    v_far = u_far_plane;

    vec4 world_pos = vec4(a_pos, 1.0); // u_model * vec4(a_pos, 1.0);

    v_normal = a_normal; // mat3(u_model) * a_normal;

    vec3 camera_pos = (u_inverse_view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    v_to_camera = normalize(camera_pos - world_pos.xyz);

    v_tex_coord = a_uv;

    gl_Position = u_view_perspective_projection * vec4(a_pos, 1.0);
}
)";

const char* mesh_renderer_fragment_shader_source = R"(
#version 430 core

layout(binding = 0) uniform sampler2D uTex;

in vec2 v_tex_coord;
in vec3 v_normal;
in vec3 v_to_camera;
in float v_near;
in float v_far;

layout(location = 0) out vec4 out_color;

void main()
{
    vec3 N = gl_FrontFacing ? v_normal : -v_normal;
    vec3 L = normalize(-v_to_camera);

    float diffuse = max(dot(N, L), 0.0);
    float ambient = 0.25;
    float diffuse_intensity = ambient + diffuse * 1.25;
    vec3 lighting = vec3(min(diffuse_intensity, 1.0f));

    vec3 base_color = texture(uTex, v_tex_coord).rgb;

    out_color = vec4(base_color * lighting, 1.0);
}
)";

const char* world_grid_vertex_shader_source = R"(
#version 430 core

const vec2 xy_plane_verts[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

layout (std430, binding = 0) buffer Scene
{
    mat4 u_view;
    mat4 u_perspective_projection;
    mat4 u_view_projection;
    mat4 u_inverse_view;
    mat4 u_inverse_projection;
    mat4 u_ortho;
    
    float u_frame_width;
    float u_frame_height;

    float u_near_plane;
    float u_far_plane;
    float u_view_space_depth;
    float u_aspect_ratio;
    float u_fov_y;
};

out vec3 v_world_pos;
out vec3 v_cam_pos;
out float v_minor_grid_step;
out float v_major_grid_step;
out float v_line_width;
out float v_view_space_depth;
out float v_near;
out float v_far;

void main()
{
    v_near = u_near_plane;
    v_far = u_far_plane;

    const float SQRT2 = 1.4142135623730951;

    float plane_scale = (u_view_space_depth) * SQRT2;
    v_cam_pos = (u_inverse_view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;

    float minor_step_base = 1.0;
    float major_step_base = 10.0;

    v_view_space_depth = u_view_space_depth;

    // thresholds
    float t1 = u_view_space_depth * 0.03;
    float t2 = u_view_space_depth * 0.3;

    float cam_height = abs(v_cam_pos.z);
    float mask1 = 1.0 - step(t1, cam_height);
    float mask10 = step(t1, cam_height) * (1.0 - step(t2, cam_height));
    float mask100 = step(t2, cam_height); 

    float step1 = 1.0;
    float step10 = 10.0;
    float step100 = 100.0;

    float scale_factor = step1 * mask1 + step10 * mask10 + step100 * mask100;
    v_minor_grid_step = minor_step_base * scale_factor;
    v_major_grid_step = major_step_base * scale_factor;
    
    v_line_width = 0.01 * pow(cam_height, 0.63);

    vec2 pos = xy_plane_verts[gl_VertexID];
    v_world_pos = vec3(pos.x * plane_scale + v_cam_pos.x, pos.y * plane_scale + v_cam_pos.y, 0.0);
    gl_Position = u_view_projection * vec4(v_world_pos, 1.0);
}
)";

const char* world_grid_fragment_shader_source = R"(
#version 430 core

in vec3 v_world_pos;
in vec3 v_cam_pos;
in float v_minor_grid_step;
in float v_major_grid_step;
in float v_line_width;
in float v_view_space_depth;
in float v_near;
in float v_far;

layout(location = 0) out vec4 out_color;

void main()
{
    const vec3 x_axis_color = vec3(0.3, 1.0, 0.3);
    const vec3 y_axis_color = vec3(1.0, 0.3, 0.3);
    const vec3 minor_grid_color = vec3(0.3);
    const vec3 major_grid_color = vec3(0.4);

    vec2 world_xy = abs(v_world_pos.xy);

    // axis lines
    vec2 axis_mask = step(world_xy, vec2(v_line_width * 2.0));
    float xy_mask = max(axis_mask.x, axis_mask.y);

    // major lines
    vec2 major_lines = step(mod(world_xy, v_major_grid_step), vec2(v_line_width * 2.0));
    float major_mask = clamp(max(major_lines.x, major_lines.y) - xy_mask, 0.0, 1.0);

    // minor lines
    vec2 minor_lines = step(mod(world_xy, v_minor_grid_step), vec2(v_line_width));
    float minor_mask = clamp(max(minor_lines.x, minor_lines.y) - major_mask, 0.0, 1.0);
    
    float final_mask = xy_mask + major_mask + minor_mask;
    
    vec3 final_color = x_axis_color * axis_mask.x + y_axis_color * axis_mask.y + minor_grid_color * minor_mask + major_grid_color * major_mask;

    // fading by camera angle
    vec3 view_dir = normalize(v_cam_pos - v_world_pos);
    float angle_factor = clamp(abs(view_dir.z) * 2.2, 0.0, 1.0);

    // fading by distance
    float dist_factor = clamp(
        1.0
        - smoothstep(v_view_space_depth * 0.85, v_view_space_depth * 0.95, length(v_cam_pos.xy - v_world_pos.xy))
        - smoothstep(v_view_space_depth * 0.45, v_view_space_depth * 0.5, v_cam_pos.z)
        , 0.0, 1.0
    );

    float final_alpha = dist_factor *  angle_factor * final_mask * 0.8;

    out_color = vec4(final_color, final_alpha);
}
)";

const char* sdf_font_vertex_shader_source = R"(
#version 430 core

layout (location = 0) in unsigned int a_glyph_index; // per-instance glyph index
layout (location = 1) in mat3x2 a_gpyph_transform;  // per-instance transform
layout (location = 4) in vec3 a_gpyph_color;  // per-instance color
layout (location = 5) in vec3 a_outline_color;  // per-instance color


// === SSBO with glyph UVs ===
struct GlyphUVs {
    vec2 uv_min;
    vec2 uv_max;
};

layout(std430, binding = 1) buffer GlyphBuffer
{
    GlyphUVs glyphs[];
};

layout (std430, binding = 0) buffer Scene
{
    mat4 u_view;
    mat4 u_perspective_projection;
    mat4 u_view_projection;
    mat4 u_inverse_view;
    mat4 u_inverse_projection;
    mat4 u_ortho;
    
    float u_frame_width;
    float u_frame_height;

    float u_near_plane;
    float u_far_plane;
    float u_view_space_depth;
    float u_aspect_ratio;
    float u_fov_y;
};

const vec2 plane_verts[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

out vec2 v_uv;
out vec3 v_color;
out vec3 v_outline_color;

void main()
{
    int vid = gl_VertexID % 4;
    vec2 local_pos = plane_verts[vid];

    GlyphUVs glyph_uvs = glyphs[a_glyph_index];

    // Interpolate UVs (simple quad mapping)
    v_uv = mix(glyph_uvs.uv_min, glyph_uvs.uv_max, local_pos);

    // Transform to clip space
    vec2 world_pos = a_gpyph_transform * vec3(local_pos, 1.0);
    gl_Position = u_ortho * vec4(world_pos, 0.0, 1.0);

    v_color = a_gpyph_color;
    v_outline_color = a_outline_color;
}
)";

const char* sdf_font_fragment_shader_source = R"(
#version 430 core

out vec4 frag_color;

layout(binding = 0) uniform sampler2D u_sdf_atlas;

in vec2 v_uv;
in vec3 v_color;
in vec3 v_outline_color;

void main()
{
    float sdf = texture(u_sdf_atlas, v_uv).r;
    float smooth_th = 0.02;
    float text_th = 0.5;
    float outline_th = 0.45;
    float text_alpha = smoothstep(text_th, text_th + smooth_th, sdf);
    float final_alpha = smoothstep(outline_th, outline_th + smooth_th, sdf);
    vec3 color = mix(v_outline_color, v_color, text_alpha);
    frag_color = vec4(color, final_alpha);
}
)";


uint32 extract_utf8_codepoint(const char* text, const char* end, const char** out)
{
    TAV_ASSERT(text);
    TAV_ASSERT(end);
    TAV_ASSERT(out);
    TAV_ASSERT(text <= end);

    if (text + 0 == end) {
        *out = end;
        return 0;
    }

    uint32 c0 = static_cast<uint8>(text[0]);
    if (!(c0 & 0x80)) {
        *out = text + 1;
        return c0;
    }

    if (text + 1 == end) {
        *out = end;
        return 0xFFFD;
    }

    uint32 c1 = static_cast<uint8>(text[1]);
    if ((c1 & 0xC0) != 0x80) {
        *out = text + 1;
        return 0xFFFD;
    }

    if ((c0 & 0xE0) == 0xC0) {
        uint32 cp = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
        *out = text + 2;
        return cp < 0x80 ? 0xFFFD : cp;
    }

    if (text + 2 == end) {
        *out = end;
        return 0xFFFD;
    }

    uint32 c2 = static_cast<uint8>(text[2]);
    if ((c2 & 0xC0) != 0x80) {
        *out = text + 1;
        return 0xFFFD;
    }

    if ((c0 & 0xF0) == 0xE0) {
        uint32 cp = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
        *out = text + 3;
        return cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF) ? 0xFFFD : cp;
    }

    if (text + 3 == end) {
        *out = end;
        return 0xFFFD;
    }

    uint32 c3 = static_cast<uint8>(text[3]);
    if ((c3 & 0xC0) != 0x80) {
        *out = text + 1;
        return 0xFFFD;
    }

    if ((c0 & 0xF8) == 0xF0) {
        uint32 cp = ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        *out = text + 4;
        return cp < 0x10000 || cp > 0x10FFFF ? 0xFFFD : cp;
    }

    *out = text + 1;
    return 0xFFFD;
}


namespace rhi = tavros::renderer::rhi;

static tavros::core::logger logger("main");

[[noreturn]] void exit_fail()
{
    std::exit(-1);
}


class render_target : tavros::core::noncopyable
{
public:
    render_target(
        rhi::graphics_device*                        graphics_device,
        tavros::core::buffer_view<rhi::pixel_format> color_attachment_formats,
        rhi::pixel_format                            depth_stencil_attachment_format
    )
        : m_graphics_device(graphics_device)
        , m_color_attachment_formats(color_attachment_formats)
        , m_depth_stencil_attachment_format(depth_stencil_attachment_format)
    {
        TAV_ASSERT(m_graphics_device);
        for (auto fmt : m_color_attachment_formats) {
            TAV_ASSERT(fmt != rhi::pixel_format::none);
        }
    }

    void recreate(uint32 width, uint32 height, uint32 msaa)
    {
        destroy();

        constexpr auto resolve_source_usage = rhi::texture_usage::resolve_source | rhi::texture_usage::render_target;
        constexpr auto resolve_destination_usage = rhi::texture_usage::resolve_destination | rhi::texture_usage::sampled | rhi::texture_usage::transfer_source;

        for (auto fmt : m_color_attachment_formats) {
            auto src_tex = create_texture(width, height, fmt, resolve_source_usage, msaa);
            m_resolve_source_color_attachments.push_back(src_tex);

            auto dst_tex = create_texture(width, height, fmt, resolve_destination_usage, 1);
            m_resolve_destination_color_attachments.push_back(dst_tex);
        }

        m_resolve_source_depth_stencil_attachment = create_texture(width, height, m_depth_stencil_attachment_format, resolve_source_usage, msaa);
        m_resolve_destination_depth_stencil_attachment = create_texture(width, height, m_depth_stencil_attachment_format, resolve_destination_usage, 1);

        m_framebuffer = create_fb(width, height, msaa);
        m_render_pass = create_rp(true);
    }

    void destroy()
    {
        for (auto tex : m_resolve_source_color_attachments) {
            m_graphics_device->destroy_texture(tex);
        }
        m_resolve_source_color_attachments.clear();

        for (auto tex : m_resolve_destination_color_attachments) {
            m_graphics_device->destroy_texture(tex);
        }
        m_resolve_destination_color_attachments.clear();

        if (m_resolve_source_depth_stencil_attachment) {
            m_graphics_device->destroy_texture(m_resolve_source_depth_stencil_attachment);
            m_resolve_source_depth_stencil_attachment = {};
        }

        if (m_resolve_destination_depth_stencil_attachment) {
            m_graphics_device->destroy_texture(m_resolve_destination_depth_stencil_attachment);
            m_resolve_destination_depth_stencil_attachment = {};
        }

        if (m_framebuffer) {
            m_graphics_device->destroy_framebuffer(m_framebuffer);
            m_framebuffer = {};
        }

        if (m_render_pass) {
            m_graphics_device->destroy_render_pass(m_render_pass);
            m_render_pass = {};
        }
    }

    uint32 color_attachment_count() const
    {
        return m_color_attachment_formats.size();
    }

    rhi::texture_handle get_color_attachment(uint32 index) const
    {
        return m_resolve_destination_color_attachments[index];
    }

    rhi::texture_handle get_depth_stencil_attachment() const
    {
        return m_resolve_destination_depth_stencil_attachment;
    }

    rhi::framebuffer_handle framebuffer() const
    {
        TAV_ASSERT(m_framebuffer);
        return m_framebuffer;
    }

    rhi::render_pass_handle render_pass() const
    {
        return m_render_pass;
    }

private:
    rhi::framebuffer_handle create_fb(uint32 width, uint32 height, uint32 msaa)
    {
        rhi::framebuffer_create_info fb_info;
        fb_info.width = width;
        fb_info.height = height;
        fb_info.color_attachments = m_resolve_source_color_attachments;
        fb_info.has_depth_stencil_attachment = true;
        fb_info.depth_stencil_attachment = m_resolve_source_depth_stencil_attachment;
        fb_info.sample_count = msaa;

        auto fb = m_graphics_device->create_framebuffer(fb_info);
        if (!fb) {
            ::logger.fatal("Failed to create framebuffer.");
            exit_fail();
        }
        return fb;
    }

    rhi::texture_handle create_texture(uint32 width, uint32 height, rhi::pixel_format fmt, tavros::core::flags<rhi::texture_usage> usage, uint32 msaa)
    {
        rhi::texture_create_info tex_info;
        tex_info.type = rhi::texture_type::texture_2d;
        tex_info.format = fmt;
        tex_info.width = width;
        tex_info.height = height;
        tex_info.depth = 1;
        tex_info.usage = usage;
        tex_info.mip_levels = 1;
        tex_info.array_layers = 1;
        tex_info.sample_count = msaa;

        auto tex = m_graphics_device->create_texture(tex_info);
        if (!tex) {
            ::logger.fatal("Failed to create texture.");
            exit_fail();
        }
        return tex;
    }

    rhi::render_pass_handle create_rp(bool need_resolve)
    {
        rhi::render_pass_create_info rp_info;

        uint32 resolve_index = 0;
        for (uint32 index = 0; index < m_color_attachment_formats.size(); ++index) {
            rhi::color_attachment_info ca_info;
            ca_info.format = m_color_attachment_formats[index];
            ca_info.load = rhi::load_op::clear;
            ca_info.store = need_resolve ? rhi::store_op::resolve : rhi::store_op::store;
            ca_info.resolve_target = need_resolve ? m_resolve_destination_color_attachments[index] : rhi::texture_handle();
            ca_info.clear_value[0] = 0.2f;
            ca_info.clear_value[1] = 0.2f;
            ca_info.clear_value[2] = 0.25f;
            ca_info.clear_value[3] = 1.0f;

            rp_info.color_attachments.push_back(ca_info);
        }

        rhi::depth_stencil_attachment_info dsca_info;
        dsca_info.format = m_depth_stencil_attachment_format;
        dsca_info.depth_load = rhi::load_op::clear;
        dsca_info.depth_store = need_resolve ? rhi::store_op::resolve : rhi::store_op::store;
        dsca_info.depth_clear_value = 1.0f;
        dsca_info.stencil_load = rhi::load_op::dont_care;
        dsca_info.stencil_store = rhi::store_op::dont_care;
        dsca_info.stencil_clear_value = 0;
        dsca_info.resolve_target = m_resolve_destination_depth_stencil_attachment;

        rp_info.depth_stencil_attachment = dsca_info;

        auto rp = m_graphics_device->create_render_pass(rp_info);
        if (!rp) {
            ::logger.fatal("Failed to create render pass.");
            exit_fail();
        }

        return rp;
    }

private:
    template<class T>
    using vector_t = tavros::core::static_vector<T, rhi::k_max_color_attachments>;

    rhi::graphics_device*         m_graphics_device;
    vector_t<rhi::pixel_format>   m_color_attachment_formats;
    rhi::pixel_format             m_depth_stencil_attachment_format;
    vector_t<rhi::texture_handle> m_resolve_source_color_attachments;
    vector_t<rhi::texture_handle> m_resolve_destination_color_attachments;
    rhi::texture_handle           m_resolve_source_depth_stencil_attachment;
    rhi::texture_handle           m_resolve_destination_depth_stencil_attachment;
    rhi::framebuffer_handle       m_framebuffer;
    rhi::render_pass_handle       m_render_pass;
};


struct font_instance_info
{
    float              mat[3][2] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    uint32             glyph_index = 0;
    tavros::math::vec3 color;
    tavros::math::vec3 outline_color;
};


class main_window : public app::render_app_base
{
public:
    main_window(tavros::core::string_view name, tavros::core::shared_ptr<tavros::resources::resource_manager> resource_manager)
        : app::render_app_base(name)
        , m_imcodec(&m_allocator)
        , m_resource_manager(resource_manager)
    {
    }

    ~main_window() override
    {
    }

    rhi::buffer_handle create_stage_buffer(size_t size, rhi::buffer_access access)
    {
        rhi::buffer_create_info info;
        info.size = size;
        info.usage = rhi::buffer_usage::stage;
        info.access = access;
        auto buffer = m_graphics_device->create_buffer(info);
        if (!buffer) {
            ::logger.fatal("Failed to create stage buffer.");
            exit_fail();
        }
        return buffer;
    }

    app::image_codec::pixels_view load_image(tavros::core::string_view path, bool y_flip = false)
    {
        tavros::core::dynamic_buffer<uint8> buffer(&m_allocator);

        auto res = m_resource_manager->open(path, tavros::resources::resource_access::read_only);
        if (res) {
            auto* reader = res->reader();
            if (reader->is_open()) {
                auto size = reader->size();
                buffer.reserve(size);
                reader->read(buffer);
            }
        }

        // buffer.data() can be nullptr; decode_image will return fallback with white pixel
        return m_imcodec.decode(buffer, 4, y_flip);
    }

    bool save_image(const app::image_codec::pixels_view& pixels, tavros::core::string_view path, bool y_flip = false)
    {
        auto im_data = m_imcodec.encode(pixels, y_flip);
        if (im_data.empty()) {
            ::logger.error("Failed to save image '{}': im_data is empty()", path);
            return false;
        }

        auto res = m_resource_manager->open(path, tavros::resources::resource_access::write_only);
        if (res) {
            auto* writer = res->writer();
            if (writer->is_open()) {
                writer->write(im_data);
                return true;
            }
        }

        ::logger.error("Failed to save image: '{}'", path);
        return false;
    }

    rhi::shader_binding_handle load_font_data(tavros::core::string_view path)
    {
        tavros::core::dynamic_buffer<uint8> font_data(&m_allocator);

        auto res = m_resource_manager->open(path);
        if (res) {
            auto* reader = res->reader();
            if (reader->is_open()) {
                auto size = reader->size();
                font_data.reserve(size);
                reader->read(font_data);
            }
        } else {
            ::logger.fatal("Failed to load font {}.", path);
            exit_fail();
        }

        m_font.init(std::move(font_data));
        if (!m_font.is_init()) {
            ::logger.error("Failed to init font.");
            exit_fail();
        }

        m_font_atlas.begin_atlas();
        for (uint32 codepoint = 0; codepoint < 0x80; ++codepoint) {
            m_font_atlas.add_glyph_from(&m_font, codepoint, 64.0f, 8.0f);
        }
        m_font_atlas.end_atlas();

        auto atlas_size = m_font_atlas.get_atlas_size();

        std::vector<uint8>                   bitmap(atlas_size.width * atlas_size.height, 0);
        tavros::ui::font_atlas::atlas_pixels pixels;
        pixels.pixels = bitmap.data();
        pixels.width = atlas_size.width;
        pixels.height = atlas_size.height;
        pixels.stride = atlas_size.width;

        if (!m_font_atlas.bake_atlas(pixels)) {
            ::logger.error("Failed to make atlas.");
            exit_fail();
        }

        // create texture
        size_t tex_size = atlas_size.width * atlas_size.height;
        auto   dst = m_graphics_device->map_buffer(m_stage_buffer);
        dst.copy_from(bitmap.data(), tex_size);
        m_graphics_device->unmap_buffer(m_stage_buffer);

        rhi::texture_create_info tex_info;
        tex_info.type = rhi::texture_type::texture_2d;
        tex_info.format = rhi::pixel_format::r8un;
        tex_info.width = atlas_size.width;
        tex_info.height = atlas_size.height;
        tex_info.depth = 1;
        tex_info.usage = rhi::k_default_texture_usage;
        tex_info.mip_levels = 1;
        tex_info.array_layers = 1;
        tex_info.sample_count = 1;

        auto texture = m_graphics_device->create_texture(tex_info);
        if (!texture) {
            ::logger.fatal("Failed to create sdf texture");
            exit_fail();
        }

        auto* cbuf = m_composer->create_command_queue();
        cbuf->wait_for_fence(m_fence);
        rhi::texture_copy_region rgn;
        rgn.width = atlas_size.width;
        rgn.height = atlas_size.height;
        cbuf->copy_buffer_to_texture(m_stage_buffer, texture, rgn);
        cbuf->signal_fence(m_fence);
        m_composer->submit_command_queue(cbuf);

        // Create SSBO

        struct glyph_uv_data
        {
            tavros::math::vec2 min;
            tavros::math::vec2 max;
        };
        std::vector<glyph_uv_data> uv_data;
        uv_data.reserve(m_font_atlas.map().size());

        uint32 index = 0;
        for (auto& item : m_font_atlas.map()) {
            auto& info = item.second;
            uv_data.push_back({item.second.uv1, item.second.uv2});
        }

        size_t                  uv_size = uv_data.size() * sizeof(glyph_uv_data);
        rhi::buffer_create_info uniform_buffer_desc{uv_size, rhi::buffer_usage::storage, rhi::buffer_access::cpu_to_gpu};
        auto                    ssbo_uv = m_graphics_device->create_buffer(uniform_buffer_desc);
        if (!ssbo_uv) {
            ::logger.fatal("Failed to create ssbo uv buffer");
            exit_fail();
        }

        cbuf->wait_for_fence(m_fence);
        auto dst_uv = m_graphics_device->map_buffer(ssbo_uv);
        dst_uv.copy_from(uv_data.data(), uv_size);
        m_graphics_device->unmap_buffer(ssbo_uv);
        cbuf->signal_fence(m_fence);

        rhi::sampler_create_info sampler_info;
        sampler_info.filter.mipmap_filter = rhi::mipmap_filter_mode::off;
        sampler_info.filter.min_filter = rhi::filter_mode::linear;
        sampler_info.filter.mag_filter = rhi::filter_mode::linear;

        auto sampler = m_graphics_device->create_sampler(sampler_info);
        if (!sampler) {
            ::logger.fatal("Failed to create sdf sampler");
            exit_fail();
        }

        rhi::shader_binding_create_info shader_binding_info;
        shader_binding_info.texture_bindings.push_back({texture, sampler, 0});
        shader_binding_info.buffer_bindings.push_back({ssbo_uv, 0, 0, 1});
        auto sh_binding = m_graphics_device->create_shader_binding(shader_binding_info);
        if (!sh_binding) {
            ::logger.fatal("Failed to create shader binding pass");
            exit_fail();
        }

        app::image_codec::pixels_view im{atlas_size.width, atlas_size.height, 1, atlas_size.width, bitmap.data()};
        save_image(im, "font_atlas.png");

        return sh_binding;
    }

    void init() override
    {
        m_camera.set_orientation({1.0f, 1.0f, -0.25f}, {0.0f, 0.0f, 1.0f});
        m_camera.set_position({-8.0f, -8.0f, 5.0f});

        m_graphics_device = rhi::graphics_device::create(rhi::render_backend_type::opengl);
        if (!m_graphics_device) {
            ::logger.fatal("Failed to create graphics_device.");
            exit_fail();
        }

        rhi::frame_composer_create_info main_composer_info;
        main_composer_info.width = 1;
        main_composer_info.height = 1;
        main_composer_info.buffer_count = 3;
        main_composer_info.vsync = true;
        main_composer_info.color_attachment_format = rhi::pixel_format::rgba8un;
        main_composer_info.depth_stencil_attachment_format = rhi::pixel_format::depth32f_stencil8;
        main_composer_info.native_handle = native_handle();

        auto main_composer_handle = m_graphics_device->create_frame_composer(main_composer_info);
        if (!main_composer_handle) {
            ::logger.fatal("Failed to create main frame composer.");
            exit_fail();
        }

        m_composer = m_graphics_device->get_frame_composer_ptr(main_composer_handle);
        if (m_composer == nullptr) {
            ::logger.fatal("Failed to get main frame composer.");
            exit_fail();
        }

        m_offscreen_rt = tavros::core::make_unique<render_target>(m_graphics_device.get(), rhi::pixel_format::rgba8un, rhi::pixel_format::depth32f);

        auto fullscreen_quad_vertex_shader = m_graphics_device->create_shader({fullscreen_quad_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto fullscreen_quad_fragment_shader = m_graphics_device->create_shader({fullscreen_quad_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info fullscreen_quad_pipeline_info;
        fullscreen_quad_pipeline_info.shaders.push_back(fullscreen_quad_vertex_shader);
        fullscreen_quad_pipeline_info.shaders.push_back(fullscreen_quad_fragment_shader);
        fullscreen_quad_pipeline_info.depth_stencil.depth_test_enable = false;
        fullscreen_quad_pipeline_info.depth_stencil.depth_write_enable = false;
        fullscreen_quad_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
        fullscreen_quad_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        fullscreen_quad_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        fullscreen_quad_pipeline_info.topology = rhi::primitive_topology::triangle_strip;
        fullscreen_quad_pipeline_info.blend_states.push_back({false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});

        m_fullscreen_quad_pipeline = m_graphics_device->create_pipeline(fullscreen_quad_pipeline_info);

        m_graphics_device->destroy_shader(fullscreen_quad_vertex_shader);
        m_graphics_device->destroy_shader(fullscreen_quad_fragment_shader);

        m_stage_buffer = create_stage_buffer(1024 * 1024 * 32, rhi::buffer_access::cpu_to_gpu);

        m_stage_upload_buffer = create_stage_buffer(1024 * 1024 * 64, rhi::buffer_access::gpu_to_cpu);

        m_fence = m_graphics_device->create_fence();
        if (!m_fence) {
            ::logger.fatal("Failed to create fence");
            exit_fail();
        }

        auto im_view = load_image("textures/cube_test.png");

        size_t tex_size = im_view.width * im_view.height * im_view.channels;
        auto   dst = m_graphics_device->map_buffer(m_stage_buffer);
        dst.copy_from(im_view.data, tex_size);
        m_graphics_device->unmap_buffer(m_stage_buffer);

        rhi::texture_create_info tex_create_info;
        tex_create_info.type = rhi::texture_type::texture_2d;
        tex_create_info.format = rhi::pixel_format::rgba8un;
        tex_create_info.width = im_view.width;
        tex_create_info.height = im_view.height;
        tex_create_info.depth = 1;
        tex_create_info.usage = rhi::k_default_texture_usage;
        tex_create_info.mip_levels = tavros::math::mip_levels(im_view.width, im_view.height);
        tex_create_info.array_layers = 1;
        tex_create_info.sample_count = 1;

        m_texture = m_graphics_device->create_texture(tex_create_info);
        if (!m_texture) {
            ::logger.fatal("Failed to create texture");
            exit_fail();
        }


        rhi::texture_copy_region copy_region;

        copy_region.width = im_view.width;
        copy_region.height = im_view.height;
        copy_region.buffer_row_length = im_view.stride / im_view.channels;

        auto* cbuf = m_composer->create_command_queue();
        cbuf->copy_buffer_to_texture(m_stage_buffer, m_texture, copy_region);
        cbuf->signal_fence(m_fence);
        m_composer->submit_command_queue(cbuf);

        rhi::sampler_create_info sampler_info;
        sampler_info.filter.mipmap_filter = rhi::mipmap_filter_mode::off;
        sampler_info.filter.min_filter = rhi::filter_mode::linear;
        sampler_info.filter.mag_filter = rhi::filter_mode::linear;

        m_sampler = m_graphics_device->create_sampler(sampler_info);
        if (!m_sampler) {
            ::logger.fatal("Failed to create sampler");
            exit_fail();
        }

        rhi::render_pass_create_info main_render_pass;
        main_render_pass.color_attachments.push_back({rhi::pixel_format::rgba8un, rhi::load_op::clear, rhi::store_op::dont_care, {}, {0.2f, 0.2f, 0.25f, 1.0f}});
        main_render_pass.depth_stencil_attachment.format = rhi::pixel_format::depth32f_stencil8;
        main_render_pass.depth_stencil_attachment.depth_load = rhi::load_op::clear;
        main_render_pass.depth_stencil_attachment.depth_store = rhi::store_op::store;
        main_render_pass.depth_stencil_attachment.depth_clear_value = 1.0f;
        main_render_pass.depth_stencil_attachment.stencil_load = rhi::load_op::clear;
        main_render_pass.depth_stencil_attachment.stencil_store = rhi::store_op::dont_care;
        main_render_pass.depth_stencil_attachment.stencil_clear_value = 0;
        m_main_pass = m_graphics_device->create_render_pass(main_render_pass);
        if (!m_main_pass) {
            ::logger.fatal("Failed to create render pass");
            exit_fail();
        }

        rhi::shader_binding_create_info shader_binding_info;
        shader_binding_info.texture_bindings.push_back({m_texture, m_sampler, 0});
        m_shader_binding = m_graphics_device->create_shader_binding(shader_binding_info);
        if (!m_shader_binding) {
            ::logger.fatal("Failed to create render pass");
            exit_fail();
        }

        rhi::buffer_create_info uniform_buffer_desc{1024ull * 1024ull, rhi::buffer_usage::storage, rhi::buffer_access::gpu_only};
        m_uniform_buffer = m_graphics_device->create_buffer(uniform_buffer_desc);
        if (!m_uniform_buffer) {
            ::logger.fatal("Failed to create uniform buffer");
            exit_fail();
        }


        auto mesh_rendering_vertex_shader = m_graphics_device->create_shader({mesh_renderer_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto mesh_rendering_fragment_shader = m_graphics_device->create_shader({mesh_renderer_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info mesh_rendering_pipeline_info;

        mesh_rendering_pipeline_info.attributes.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0});
        mesh_rendering_pipeline_info.attributes.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 1});
        mesh_rendering_pipeline_info.attributes.push_back({rhi::attribute_type::vec2, rhi::attribute_format::f32, false, 2});

        mesh_rendering_pipeline_info.shaders.push_back(mesh_rendering_vertex_shader);
        mesh_rendering_pipeline_info.shaders.push_back(mesh_rendering_fragment_shader);
        mesh_rendering_pipeline_info.depth_stencil.depth_test_enable = true;
        mesh_rendering_pipeline_info.depth_stencil.depth_write_enable = true;
        mesh_rendering_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
        mesh_rendering_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        mesh_rendering_pipeline_info.rasterizer.face = rhi::front_face::counter_clockwise;
        mesh_rendering_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        mesh_rendering_pipeline_info.topology = rhi::primitive_topology::triangles;
        mesh_rendering_pipeline_info.blend_states.push_back({false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
        mesh_rendering_pipeline_info.multisample.sample_shading_enabled = false;
        mesh_rendering_pipeline_info.multisample.sample_count = 1;
        mesh_rendering_pipeline_info.multisample.min_sample_shading = 0.0;

        m_mesh_rendering_pipeline = m_graphics_device->create_pipeline(mesh_rendering_pipeline_info);
        if (!m_mesh_rendering_pipeline) {
            ::logger.fatal("Failed to create mesh rendering pipeline");
            exit_fail();
        }

        m_graphics_device->destroy_shader(mesh_rendering_vertex_shader);
        m_graphics_device->destroy_shader(mesh_rendering_fragment_shader);


        rhi::buffer_create_info mesh_vertices_buffer_info{1024 * 1024 * 16, rhi::buffer_usage::vertex, rhi::buffer_access::cpu_to_gpu};
        auto                    mesh_vertices_buffer = m_graphics_device->create_buffer(mesh_vertices_buffer_info);
        if (!mesh_vertices_buffer) {
            ::logger.fatal("Failed to create mesh vertices buffer");
            exit_fail();
        }

        rhi::buffer_create_info mesh_indices_buffer_info{1024 * 128, rhi::buffer_usage::index, rhi::buffer_access::cpu_to_gpu};
        auto                    mesh_indices_buffer = m_graphics_device->create_buffer(mesh_indices_buffer_info);
        if (!mesh_indices_buffer) {
            ::logger.fatal("Failed to create mesh indices buffer");
            exit_fail();
        }

        rhi::geometry_create_info mesh_geometry_info;
        mesh_geometry_info.vertex_buffer_layouts.push_back({mesh_vertices_buffer, 0, sizeof(app::vertex_type)});
        mesh_geometry_info.attribute_bindings.push_back({0, offsetof(app::vertex_type, app::vertex_type::pos), 0, rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0});
        mesh_geometry_info.attribute_bindings.push_back({0, offsetof(app::vertex_type, app::vertex_type::normal), 0, rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 1});
        mesh_geometry_info.attribute_bindings.push_back({0, offsetof(app::vertex_type, app::vertex_type::uv), 0, rhi::attribute_type::vec2, rhi::attribute_format::f32, false, 2});
        mesh_geometry_info.has_index_buffer = true;
        mesh_geometry_info.index_format = rhi::index_buffer_format::u32;
        mesh_geometry_info.index_buffer = mesh_indices_buffer;

        m_mesh_geometry = m_graphics_device->create_geometry(mesh_geometry_info);
        if (!m_mesh_geometry) {
            ::logger.fatal("Failed to create mesh geometry");
            exit_fail();
        }

        auto verts_map = m_graphics_device->map_buffer(mesh_vertices_buffer);
        verts_map.copy_from(app::cube_vertices, sizeof(app::cube_vertices));
        m_graphics_device->unmap_buffer(mesh_vertices_buffer);

        auto inds_map = m_graphics_device->map_buffer(mesh_indices_buffer);
        inds_map.copy_from(app::cube_indices, sizeof(app::cube_indices));
        m_graphics_device->unmap_buffer(mesh_indices_buffer);

        rhi::shader_binding_create_info mesh_shader_binding_info;
        mesh_shader_binding_info.texture_bindings.push_back({m_texture, m_sampler, 0});

        m_mesh_shader_binding = m_graphics_device->create_shader_binding(mesh_shader_binding_info);
        if (!m_mesh_shader_binding) {
            ::logger.fatal("Failed to create shader binding");
            exit_fail();
        }


        // World grid
        auto world_grid_rendering_vertex_shader = m_graphics_device->create_shader({world_grid_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto world_grid_rendering_fragment_shader = m_graphics_device->create_shader({world_grid_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info world_grid_rendering_pipeline_info;
        world_grid_rendering_pipeline_info.shaders.push_back(world_grid_rendering_vertex_shader);
        world_grid_rendering_pipeline_info.shaders.push_back(world_grid_rendering_fragment_shader);
        world_grid_rendering_pipeline_info.depth_stencil.depth_test_enable = true;
        world_grid_rendering_pipeline_info.depth_stencil.depth_write_enable = false;
        world_grid_rendering_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
        world_grid_rendering_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        world_grid_rendering_pipeline_info.rasterizer.face = rhi::front_face::counter_clockwise;
        world_grid_rendering_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        world_grid_rendering_pipeline_info.topology = rhi::primitive_topology::triangle_strip;
        world_grid_rendering_pipeline_info.blend_states.push_back({true, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
        world_grid_rendering_pipeline_info.multisample.sample_shading_enabled = false;
        world_grid_rendering_pipeline_info.multisample.sample_count = 1;
        world_grid_rendering_pipeline_info.multisample.min_sample_shading = 0.0;

        m_world_grid_rendering_pipeline = m_graphics_device->create_pipeline(world_grid_rendering_pipeline_info);
        if (!m_world_grid_rendering_pipeline) {
            ::logger.fatal("Failed to create world grid rendering pipeline");
            exit_fail();
        }

        m_graphics_device->destroy_shader(world_grid_rendering_vertex_shader);
        m_graphics_device->destroy_shader(world_grid_rendering_fragment_shader);


        rhi::shader_binding_create_info scene_shader_binding_info;
        scene_shader_binding_info.buffer_bindings.push_back({m_uniform_buffer, 0, sizeof(frame_data), 0});

        m_scene_binding = m_graphics_device->create_shader_binding(scene_shader_binding_info);
        if (!m_scene_binding) {
            ::logger.fatal("Failed to create world grid shader binding");
            exit_fail();
        }

        m_graphics_device->wait_for_fence(m_fence);

        show();

        m_font_shader_binding = load_font_data("fonts/Alice/Alice-Regular.ttf");

        // SDF
        auto sdf_font_vertex_shader = m_graphics_device->create_shader({sdf_font_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto sdf_font_fragment_shader = m_graphics_device->create_shader({sdf_font_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info sdf_font_pipeline_info;
        sdf_font_pipeline_info.attributes.push_back({rhi::attribute_type::scalar, rhi::attribute_format::u32, false, 0});
        sdf_font_pipeline_info.attributes.push_back({rhi::attribute_type::mat3x2, rhi::attribute_format::f32, false, 1});
        sdf_font_pipeline_info.attributes.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 4});
        sdf_font_pipeline_info.attributes.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 5});
        sdf_font_pipeline_info.shaders.push_back(sdf_font_vertex_shader);
        sdf_font_pipeline_info.shaders.push_back(sdf_font_fragment_shader);
        sdf_font_pipeline_info.depth_stencil.depth_test_enable = false;
        sdf_font_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        sdf_font_pipeline_info.rasterizer.face = rhi::front_face::counter_clockwise;
        sdf_font_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        sdf_font_pipeline_info.topology = rhi::primitive_topology::triangle_strip;
        sdf_font_pipeline_info.blend_states.push_back({true, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
        sdf_font_pipeline_info.multisample.sample_shading_enabled = false;
        sdf_font_pipeline_info.multisample.sample_count = 1;
        sdf_font_pipeline_info.multisample.min_sample_shading = 0.0;

        m_sdf_font_pipeline = m_graphics_device->create_pipeline(sdf_font_pipeline_info);
        if (!m_sdf_font_pipeline) {
            ::logger.fatal("Failed to create sdf font rendering pipeline");
            exit_fail();
        }

        rhi::buffer_create_info font_instance_buffer_info{1024 * 1024, rhi::buffer_usage::vertex, rhi::buffer_access::cpu_to_gpu};
        m_font_buffer_info = m_graphics_device->create_buffer(font_instance_buffer_info);
        if (!m_font_buffer_info) {
            ::logger.fatal("Failed to create font instance buffer");
            exit_fail();
        }

        rhi::geometry_create_info font_instance_data_info;
        font_instance_data_info.vertex_buffer_layouts.push_back({m_font_buffer_info, 0, sizeof(font_instance_info)});
        font_instance_data_info.attribute_bindings.push_back({0, offsetof(font_instance_info, font_instance_info::glyph_index), 1, rhi::attribute_type::scalar, rhi::attribute_format::u32, false, 0});
        font_instance_data_info.attribute_bindings.push_back({0, offsetof(font_instance_info, font_instance_info::mat), 1, rhi::attribute_type::mat3x2, rhi::attribute_format::f32, false, 1});
        font_instance_data_info.attribute_bindings.push_back({0, offsetof(font_instance_info, font_instance_info::color), 1, rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 4});
        font_instance_data_info.attribute_bindings.push_back({0, offsetof(font_instance_info, font_instance_info::outline_color), 1, rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 5});
        font_instance_data_info.has_index_buffer = false;

        m_font_geometry = m_graphics_device->create_geometry(font_instance_data_info);
        if (!m_font_geometry) {
            ::logger.fatal("Failed to create font geometry instance binding");
            exit_fail();
        }
    }

    void shutdown() override
    {
        m_graphics_device = nullptr;
    }

    void process_events(app::event_queue_view events, double delta_time)
    {
        m_input_manager.on_frame_started(tavros::system::application::instance().highp_time_us());

        bool need_resize = false;

        // Process all events
        for (auto& it : events) {
            switch (it.type) {
            case app::event_type::key_down:
                m_input_manager.on_key_press(it.key_info, it.event_time_us);
                break;

            case app::event_type::key_up:
                m_input_manager.on_key_release(it.key_info, it.event_time_us);
                break;

            case app::event_type::mouse_move:
                m_input_manager.on_mouse_move(it.vec_info, it.event_time_us);
                break;

            case app::event_type::mouse_button_down:
                break;

            case app::event_type::mouse_button_up:
                break;

            case app::event_type::window_resize:
                m_current_frame_size = tavros::math::ivec2(static_cast<int32>(it.vec_info.width), static_cast<int32>(it.vec_info.height));
                need_resize = true;
                break;

            case app::event_type::deactivate:
                m_input_manager.clear_state();
                break;

            case app::event_type::activate:
                m_input_manager.clear_state();
                break;

            default:
                break;
            }
        }

        if (m_input_manager.is_key_released(tavros::system::keys::k_F1)) {
            if (m_current_buffer_output_index != 0) {
                m_current_buffer_output_index = 0;
            }
        } else if (m_input_manager.is_key_released(tavros::system::keys::k_F2)) {
            if (m_current_buffer_output_index != 1) {
                m_current_buffer_output_index = 1;
            }
        }

        if (m_input_manager.is_key_pressed(tavros::system::keys::k_minus)) {
            m_font_height -= 2.0f;
        }

        if (m_input_manager.is_key_pressed(tavros::system::keys::k_equal)) {
            m_font_height += 2.0f;
        }

        if (need_resize && m_current_frame_size.width != 0 && m_current_frame_size.height != 0) {
            m_offscreen_rt->recreate(static_cast<uint32>(m_current_frame_size.width), static_cast<uint32>(m_current_frame_size.height), 32);

            if (m_color_shader_binding) {
                m_graphics_device->destroy_shader_binding(m_color_shader_binding);
                m_color_shader_binding = rhi::shader_binding_handle();
            }

            if (m_depth_stencil_shader_binding) {
                m_graphics_device->destroy_shader_binding(m_depth_stencil_shader_binding);
                m_depth_stencil_shader_binding = rhi::shader_binding_handle();
            }

            rhi::shader_binding_create_info color_quad_shader_binding_info;
            color_quad_shader_binding_info.texture_bindings.push_back({m_offscreen_rt->get_color_attachment(0), m_sampler, 0});
            m_color_shader_binding = m_graphics_device->create_shader_binding(color_quad_shader_binding_info);
            if (!m_color_shader_binding) {
                ::logger.fatal("Failed to create color quad shader binding");
                exit_fail();
            }

            rhi::shader_binding_create_info depth_stencil_quad_shader_binding_info;
            depth_stencil_quad_shader_binding_info.texture_bindings.push_back({m_offscreen_rt->get_depth_stencil_attachment(), m_sampler, 0});
            m_depth_stencil_shader_binding = m_graphics_device->create_shader_binding(depth_stencil_quad_shader_binding_info);
            if (!m_depth_stencil_shader_binding) {
                ::logger.fatal("Failed to create depth/stencil quad shader binding");
                exit_fail();
            }

            m_composer->resize(m_current_frame_size.width, m_current_frame_size.height);

            constexpr float fov_y = 60.0f * 3.14159265358979f / 180.0f; // 60 deg
            float           aspect_ratio = static_cast<float>(m_current_frame_size.width) / static_cast<float>(m_current_frame_size.height);
            m_camera.set_perspective(fov_y, aspect_ratio, 0.1f, 1000.0f);
        }

        // Update camera
        auto factor = [&](tavros::system::keys key) -> float {
            return static_cast<float>(m_input_manager.key_pressed_factor(key));
        };

        // clang-format off
        tavros::math::vec3 move_delta =
            m_camera.forward() * factor(tavros::system::keys::k_W)
            - m_camera.forward() * factor(tavros::system::keys::k_S)
            + m_camera.right() * factor(tavros::system::keys::k_A)
            - m_camera.right() * factor(tavros::system::keys::k_D) 
            + m_camera.up() * factor(tavros::system::keys::k_space) 
            - m_camera.up() * factor(tavros::system::keys::k_C);
        // clang-format on

        float len = tavros::math::length(move_delta);
        if (len > 1.0f) {
            move_delta /= len;
        }

        bool  is_shift_pressed = m_input_manager.is_key_pressed(tavros::system::keys::k_lshift);
        bool  is_control_pressed = m_input_manager.is_key_pressed(tavros::system::keys::k_lcontrol);
        float speed_factor = static_cast<float>(delta_time) * (is_shift_pressed ? 10.0f * (is_control_pressed ? 10.0f : 1.0f) : 2.0f);
        m_camera.move(move_delta * speed_factor);

        // Update camera rotation
        auto mouse_delta = m_input_manager.get_smooth_mouse_delta();

        if (tavros::math::squared_length(mouse_delta) > 0.0f) {
            constexpr float base_sensitivity = 0.5f;
            auto            scaled_mouse_delta = mouse_delta * base_sensitivity;

            auto world_up = m_camera.world_up();

            auto q_yaw = tavros::math::quat::from_axis_angle(world_up, scaled_mouse_delta.x);
            auto q_pitch = tavros::math::quat::from_axis_angle(m_camera.right(), -scaled_mouse_delta.y);
            auto yaw_pitch_rotation = tavros::math::normalize(q_yaw * q_pitch);

            auto candidate_forward = tavros::math::normalize(yaw_pitch_rotation * m_camera.forward());

            auto current_forward_dot_up = tavros::math::dot(m_camera.forward(), world_up);
            auto forward_in_horizontal_plane = tavros::math::normalize(tavros::math::cross(m_camera.right(), world_up));
            auto candidate_dot_horizontal_forward = tavros::math::dot(candidate_forward, forward_in_horizontal_plane);

            if (candidate_dot_horizontal_forward < 0.000001f) {
                constexpr float cos_threshold = 0.000001f;
                auto            constrained_forward = forward_in_horizontal_plane * cos_threshold + (current_forward_dot_up > 0 ? world_up : -world_up);
                m_camera.set_orientation(q_yaw * constrained_forward, world_up);
                m_camera.rotate(q_yaw);
            } else {
                m_camera.set_orientation(candidate_forward, world_up);
            }
        }
    }

    void update_scene()
    {
        auto w = static_cast<float>(m_current_frame_size.width);
        auto h = static_cast<float>(m_current_frame_size.height);
        m_renderer_frame_data.view = m_camera.get_view_matrix();
        m_renderer_frame_data.perspective_projection = m_camera.get_projection_matrix();
        m_renderer_frame_data.view_projection = m_camera.get_view_projection_matrix();
        m_renderer_frame_data.inverse_view = tavros::math::inverse(m_camera.get_view_matrix());
        m_renderer_frame_data.inverse_projection = tavros::math::inverse(m_camera.get_projection_matrix());
        m_renderer_frame_data.orto_projection = tavros::math::mat4::ortho(0.0f, w, h, 0.0f, 1.0f, -1.0f);
        m_renderer_frame_data.frame_width = w;
        m_renderer_frame_data.frame_height = h;
        m_renderer_frame_data.near_plane = m_camera.near_plane();
        m_renderer_frame_data.far_plane = m_camera.far_plane();
        m_renderer_frame_data.view_space_depth = m_camera.far_plane() - m_camera.near_plane();
        m_renderer_frame_data.aspect_ratio = m_camera.aspect();
        m_renderer_frame_data.fov_y = m_camera.fov_y();
    }

    void render(app::event_queue_view events, double delta_time) override
    {
        process_events(events, delta_time);

        update_scene();

        auto is_f10_released = m_input_manager.is_key_released(tavros::system::keys::k_F10);

        // update
        auto uniform_buffer_data_map = m_graphics_device->map_buffer(m_stage_buffer, 0, sizeof(m_renderer_frame_data));
        uniform_buffer_data_map.copy_from(&m_renderer_frame_data, sizeof(m_renderer_frame_data));
        m_graphics_device->unmap_buffer(m_stage_buffer);

        // Update text

        const char* text = "Hello, AVAWAT WAorld!;";
        size_t      text_size = std::strlen(text);
        const char* beg = text;
        const char* tmp = text;
        const char* end = text + text_size;
        float       font_height_in_pixels = m_font_height; // Font height will be 120 pixels

        std::vector<font_instance_info> instance_info;
        auto                            aspect = m_camera.aspect();

        tavros::math::vec2 pen = tavros::math::vec2(100.0f, m_current_frame_size.height - 100.0f);

        auto cp_cur = extract_utf8_codepoint(beg, end, &beg);

        while (beg < end) {
            auto cp_next = extract_utf8_codepoint(beg, end, &beg);

            auto it = m_font_atlas.map().find(cp_cur);
            if (it == m_font_atlas.map().end()) {
                it = m_font_atlas.map().find(0);
                if (it == m_font_atlas.map().end()) {
                    cp_cur = cp_next;
                    continue;
                }
            }

            const auto& g = it->second.metrics;

            float font_scale = 2.0f * font_height_in_pixels;

            if (cp_cur == ' ') {
                pen.x += (g.advance_x * font_scale);
                cp_cur = cp_next;
                continue;
            }

            font_instance_info inst_info{};
            inst_info.glyph_index = it->second.serial_index;

            inst_info.color = tavros::math::vec3(1.0f);
            inst_info.outline_color = tavros::math::vec3(1.0f, 0.0f, 0.0f);

            // Размер quad’a
            auto pad = it->second.sdf_padding / it->second.glyph_scale * font_scale;
            inst_info.mat[0][0] = g.width * font_scale + pad * 2;
            inst_info.mat[1][1] = g.height * font_scale + pad * 2;
            // inst_info.mat[1][0] = -g.height * font_scale / 4.0f;

            // Позиция quad’a (учитываем bearing)
            inst_info.mat[2][0] = pen.x + g.bearing_x * font_scale - pad;
            inst_info.mat[2][1] = pen.y - pad - (g.height - g.bearing_y) * font_scale; // bearing_y от baseline вверх

            instance_info.push_back(inst_info);

            // === advance & kerning ===
            float kern = 0.0f;
            if (auto it_next = m_font_atlas.map().find(cp_next); it_next != m_font_atlas.map().end()) {
                kern = it->second.font->get_kerning(it->second.glyph, it_next->second.glyph) * 2.0;
            }

            pen.x += (g.advance_x + kern) * font_scale;

            cp_cur = cp_next;
        }

        size_t size = instance_info.size() * sizeof(font_instance_info);
        auto   map_data = m_graphics_device->map_buffer(m_font_buffer_info, 0, size);
        map_data.copy_from(instance_info.data(), size);
        m_graphics_device->unmap_buffer(m_font_buffer_info);

        auto* cbuf = m_composer->create_command_queue();
        m_composer->begin_frame();

        // Copy m_renderer_frame_data to shader
        cbuf->copy_buffer(m_stage_buffer, m_uniform_buffer, sizeof(m_renderer_frame_data));

        cbuf->begin_render_pass(m_offscreen_rt->render_pass(), m_offscreen_rt->framebuffer());

        // Draw cube
        cbuf->bind_pipeline(m_mesh_rendering_pipeline);
        cbuf->bind_geometry(m_mesh_geometry);
        cbuf->bind_shader_binding(m_scene_binding);
        cbuf->bind_shader_binding(m_mesh_shader_binding);
        cbuf->draw_indexed(6 * 6);

        // Draw world grid
        cbuf->bind_pipeline(m_world_grid_rendering_pipeline);
        cbuf->bind_shader_binding(m_scene_binding);
        cbuf->draw(4);


        cbuf->bind_pipeline(m_sdf_font_pipeline);
        cbuf->bind_shader_binding(m_font_shader_binding);
        cbuf->bind_shader_binding(m_scene_binding);
        cbuf->bind_geometry(m_font_geometry);
        cbuf->draw(4, 0, text_size, 0);


        cbuf->end_render_pass();

        if (is_f10_released) {
            rhi::texture_copy_region copy_rgn;
            copy_rgn.width = m_current_frame_size.width;
            copy_rgn.height = m_current_frame_size.height;
            cbuf->copy_texture_to_buffer(m_offscreen_rt->get_color_attachment(0), m_stage_upload_buffer, copy_rgn);
            copy_rgn.buffer_offset = copy_rgn.width * copy_rgn.height * 4;
            cbuf->copy_texture_to_buffer(m_offscreen_rt->get_depth_stencil_attachment(), m_stage_upload_buffer, copy_rgn);
        }

        // Draw to backbuffer
        cbuf->begin_render_pass(m_main_pass, m_composer->backbuffer());

        cbuf->bind_pipeline(m_fullscreen_quad_pipeline);
        if (m_current_buffer_output_index == 0) {
            cbuf->bind_shader_binding(m_color_shader_binding);
        } else {
            cbuf->bind_shader_binding(m_depth_stencil_shader_binding);
        }
        cbuf->draw(4);

        cbuf->end_render_pass();

        m_composer->submit_command_queue(cbuf);
        m_composer->end_frame();
        m_composer->wait_for_frame_complete();
        m_composer->present();

        if (is_f10_released) {
            auto screenshot_pixels = m_graphics_device->map_buffer(m_stage_upload_buffer);

            int width = static_cast<int>(m_current_frame_size.width);
            int height = static_cast<int>(m_current_frame_size.height);
            int channels = 4;
            int stride = width * channels;

            tavros::core::dynamic_buffer<uint8> depth_data(&m_allocator);
            depth_data.reserve(width * height);
            size_t       plane_size = static_cast<size_t>(width * height);
            const float* src = reinterpret_cast<const float*>(screenshot_pixels.data() + plane_size * channels);
            for (size_t i = 0; i < plane_size; ++i) {
                depth_data.data()[i] = static_cast<uint8>(src[i] * 255.0f);
            }

            // Save screenshot
            app::image_codec::pixels_view im_color{static_cast<uint32>(width), static_cast<uint32>(height), channels, stride, screenshot_pixels.data()};
            save_image(im_color, "color.png", true);
            app::image_codec::pixels_view im_depth{width, height, 1, width, depth_data.data()};
            save_image(im_depth, "depth.png", true);

            m_graphics_device->unmap_buffer(m_stage_upload_buffer);
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(330));
    }

private:
    float m_font_height = 100.0f;

    tavros::core::mallocator                       m_allocator;
    tavros::core::unique_ptr<rhi::graphics_device> m_graphics_device;
    rhi::frame_composer*                           m_composer = nullptr;

    app::image_codec m_imcodec;

    tavros::core::unique_ptr<render_target> m_offscreen_rt;

    rhi::pipeline_handle       m_fullscreen_quad_pipeline;
    rhi::pipeline_handle       m_mesh_rendering_pipeline;
    rhi::pipeline_handle       m_world_grid_rendering_pipeline;
    rhi::pipeline_handle       m_sdf_font_pipeline;
    rhi::render_pass_handle    m_main_pass;
    rhi::shader_binding_handle m_shader_binding;
    rhi::shader_binding_handle m_mesh_shader_binding;
    rhi::shader_binding_handle m_scene_binding;
    rhi::shader_binding_handle m_color_shader_binding;
    rhi::shader_binding_handle m_depth_stencil_shader_binding;
    rhi::shader_binding_handle m_font_shader_binding;
    rhi::texture_handle        m_texture;
    rhi::buffer_handle         m_stage_buffer;
    rhi::buffer_handle         m_stage_upload_buffer;
    rhi::sampler_handle        m_sampler;
    rhi::buffer_handle         m_uniform_buffer;
    rhi::buffer_handle         m_font_buffer_info;
    rhi::geometry_handle       m_mesh_geometry;
    rhi::geometry_handle       m_font_geometry;
    rhi::fence_handle          m_fence;

    uint32 m_current_buffer_output_index = 0;

    tavros::math::ivec2 m_current_frame_size;

    tavros::core::shared_ptr<tavros::resources::resource_manager> m_resource_manager;

    app::input_manager       m_input_manager;
    tavros::renderer::camera m_camera;

    struct alignas(16) frame_data
    {
        tavros::math::mat4 view;
        tavros::math::mat4 perspective_projection;
        tavros::math::mat4 view_projection;
        tavros::math::mat4 inverse_view;
        tavros::math::mat4 inverse_projection;
        tavros::math::mat4 orto_projection;

        float frame_width = 0.0f;
        float frame_height = 0.0f;

        float near_plane = 0.0f;
        float far_plane = 0.0f;
        float view_space_depth = 0.0f;
        float aspect_ratio = 0.0f;
        float fov_y = 0.0f;
    };

    frame_data m_renderer_frame_data;

    tavros::ui::truetype_font m_font;
    tavros::ui::font_atlas    m_font_atlas;
};

int main()
{
    tavros::core::logger::add_consumer([](auto lvl, auto tag, auto msg) { printf("%s\n", msg.data()); });

    auto resource_manager = tavros::core::make_shared<tavros::resources::resource_manager>();
    resource_manager->mount<tavros::resources::filesystem_provider>("C:/Users/Piotr/Desktop/Tavros/assets", tavros::resources::resource_access::read_only);
    resource_manager->mount<tavros::resources::filesystem_provider>("C:/Work/q3pp_res/baseq3", tavros::resources::resource_access::read_only);
    resource_manager->mount<tavros::resources::filesystem_provider>("C:/Users/Piotr/Desktop/TavrosOutput", tavros::resources::resource_access::write_only);

    auto wnd = tavros::core::make_unique<main_window>("TavrosEngine", resource_manager);
    wnd->run_render_loop();

    return tavros::system::application::instance().run();
}
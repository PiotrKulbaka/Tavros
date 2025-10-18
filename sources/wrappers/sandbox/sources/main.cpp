#include <stdio.h>

#include "render_app_base.hpp"
#include "image_decoder.hpp"
#include "built_in_meshes.hpp"
#include "input_manager.hpp"

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/renderer/rhi/command_list.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/render_system.hpp>
#include <tavros/renderer/camera/camera.hpp>
#include <tavros/renderer/render_target.hpp>
#include <tavros/core/memory/memory.hpp>

#include <tavros/resources/resource_manager.hpp>
#include <tavros/resources/providers/filesystem_provider.hpp>
#include <tavros/core/memory/buffer.hpp>

#include <tavros/system/time.hpp>

#include <algorithm>

#include <stb/stb_image_write.h>

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
layout (location = 2) in vec2 a_uv_outside;
layout (location = 3) in vec2 a_uv_inside;

layout (std430, binding = 0) buffer Scene
{
    mat4 u_view;
    mat4 u_perspective_projection;
    mat4 u_view_perspective_projection;
    mat4 u_inverse_view;
    mat4 u_inverse_perspective_projection;

    float u_frame_width;
    float u_frame_height;

    float u_near_plane;
    float u_far_plane;
    float u_view_space_depth;
    float u_aspect_ratio;
    float u_fov_y;
};

out vec2 v_tex_coord_outside;
out vec2 v_tex_coord_inside;
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

    v_tex_coord_outside = a_uv_outside;
    v_tex_coord_inside = a_uv_inside;

    gl_Position = u_view_perspective_projection * vec4(a_pos, 1.0);
}
)";

const char* mesh_renderer_fragment_shader_source = R"(
#version 430 core

layout(binding = 0) uniform sampler2D uTex;

in vec2 v_tex_coord_outside;
in vec2 v_tex_coord_inside;
in vec3 v_normal;
in vec3 v_to_camera;
in float v_near;
in float v_far;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_depth;

float linear_depth01(float z_buffer, float near, float far)
{
    float z_eye = (2.0 * near * far) / (far + near - z_buffer * (far - near));
    return (z_eye - near) / (far - near);
}

void main()
{
    vec3 N = gl_FrontFacing ? -v_normal : v_normal;
    vec3 L = normalize(-v_to_camera);

    float diffuse = max(dot(N, L), 0.0);
    float ambient = 0.25;
    float diffuse_intensity = ambient + diffuse * 1.25;
    vec3 lighting = vec3(min(diffuse_intensity, 1.0f));

    vec2 texCoord = gl_FrontFacing ? v_tex_coord_outside : v_tex_coord_inside;
    vec3 base_color = texture(uTex, texCoord).rgb;

    out_color = vec4(base_color * lighting, 1.0);

    float linear_depth = linear_depth01(gl_FragCoord.z, v_near, v_far);

    out_depth = vec4(vec3(linear_depth), 1.0);
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

out vec4 FragColor;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_depth;

float linear_depth01(float z_buffer, float near, float far)
{
    float z_eye = (2.0 * near * far) / (far + near - z_buffer * (far - near));
    return (z_eye - near) / (far - near);
}

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

    float linear_depth = linear_depth01(gl_FragCoord.z, v_near, v_far);

    out_depth = vec4(vec3(linear_depth), 1.0);
}
)";


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

        m_framebuffer = create_framebuffer(width, height, msaa);
        m_render_pass = create_render_pass(msaa, true);
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
        if (m_resolve_destination_color_attachments.size()) {
            return m_resolve_destination_color_attachments[index];
        }
        return m_resolve_source_color_attachments[index];
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
    rhi::framebuffer_handle create_framebuffer(uint32 width, uint32 height, uint32 msaa)
    {
        rhi::framebuffer_create_info fb_info;
        fb_info.width = width;
        fb_info.height = height;
        fb_info.color_attachment_formats = m_color_attachment_formats;
        fb_info.depth_stencil_attachment_format = m_depth_stencil_attachment_format;
        fb_info.sample_count = msaa;

        auto fb = m_graphics_device->create_framebuffer(fb_info, m_resolve_source_color_attachments, m_resolve_source_depth_stencil_attachment);
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

    rhi::render_pass_handle create_render_pass(uint32 msaa, bool need_resolve)
    {
        rhi::render_pass_create_info rp_info;

        uint32 resolve_index = 0;
        for (uint32 index = 0; index < m_color_attachment_formats.size(); ++index) {
            rhi::color_attachment_info ca_info;
            ca_info.format = m_color_attachment_formats[index];
            ca_info.sample_count = msaa;
            ca_info.load = rhi::load_op::clear;
            ca_info.store = need_resolve ? rhi::store_op::resolve : rhi::store_op::store;
            ca_info.resolve_texture_index = index;
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

        rp_info.depth_stencil_attachment = dsca_info;

        tavros::core::buffer_view<rhi::texture_handle> resolve_textures;
        if (need_resolve) {
            resolve_textures = m_resolve_destination_color_attachments;
        }

        auto rp = m_graphics_device->create_render_pass(rp_info, resolve_textures);
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


class my_app : public app::render_app_base
{
public:
    my_app(tavros::core::shared_ptr<tavros::resources::resource_manager> resource_manager)
        : app::render_app_base("TavrosEngine")
        , m_image_decoder(&m_allocator)
        , m_resource_manager(resource_manager)
    {
    }

    ~my_app() override
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

    app::image_decoder::pixels_view load_image(tavros::core::string_view path)
    {
        tavros::core::dynamic_buffer<uint8> buffer(&m_allocator);

        auto res = m_resource_manager->open(path);
        if (res) {
            auto* reader = res->reader();
            if (reader->is_open()) {
                auto size = reader->size();
                buffer.reserve(size);
                reader->read(buffer);
            }
        }

        // buffer.data() can be nullptr; decode_image will return fallback with white pixel
        return m_image_decoder.decode_image(buffer);
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

        auto main_composer_handle = m_graphics_device->create_frame_composer(main_composer_info, native_window_handle());
        if (!main_composer_handle) {
            ::logger.fatal("Failed to create main frame composer.");
            exit_fail();
        }

        m_composer = m_graphics_device->get_frame_composer_ptr(main_composer_handle);
        if (m_composer == nullptr) {
            ::logger.fatal("Failed to get main frame composer.");
            exit_fail();
        }

        tavros::core::static_vector<rhi::pixel_format, rhi::k_max_color_attachments> color_attachment_formats;
        color_attachment_formats.push_back(rhi::pixel_format::rgba8un);
        color_attachment_formats.push_back(rhi::pixel_format::rgba8un);
        rhi::pixel_format depth_stencil_attachment_format = rhi::pixel_format::depth32f_stencil8;
        m_offscreen_rt = tavros::core::make_unique<render_target>(m_graphics_device.get(), color_attachment_formats, depth_stencil_attachment_format);

        auto fullscreen_quad_vertex_shader = m_graphics_device->create_shader({fullscreen_quad_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto fullscreen_quad_fragment_shader = m_graphics_device->create_shader({fullscreen_quad_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info fullscreen_quad_pipeline_info;
        fullscreen_quad_pipeline_info.shaders.push_back({rhi::shader_stage::vertex, "main"});
        fullscreen_quad_pipeline_info.shaders.push_back({rhi::shader_stage::fragment, "main"});
        fullscreen_quad_pipeline_info.depth_stencil.depth_test_enable = false;
        fullscreen_quad_pipeline_info.depth_stencil.depth_write_enable = false;
        fullscreen_quad_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
        fullscreen_quad_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        fullscreen_quad_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        fullscreen_quad_pipeline_info.topology = rhi::primitive_topology::triangle_strip;
        fullscreen_quad_pipeline_info.blend_states.push_back({false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});

        rhi::shader_handle fullscreen_quad_shaders[] = {fullscreen_quad_vertex_shader, fullscreen_quad_fragment_shader};
        m_fullscreen_quad_pipeline = m_graphics_device->create_pipeline(fullscreen_quad_pipeline_info, fullscreen_quad_shaders);

        m_stage_buffer = create_stage_buffer(1024 * 1024 * 16, rhi::buffer_access::cpu_to_gpu);

        m_stage_upload_buffer = create_stage_buffer(1024 * 1024 * 32, rhi::buffer_access::gpu_to_cpu);

        auto im_view = load_image("textures/cube_test.png");

        size_t tex_size = im_view.width * im_view.height * im_view.channels;
        auto   dst = m_graphics_device->map_buffer(m_stage_buffer);
        memcpy(dst.data(), im_view.data, tex_size);
        m_graphics_device->unmap_buffer(m_stage_buffer);

        rhi::texture_create_info tex_create_info;
        tex_create_info.type = rhi::texture_type::texture_2d;
        tex_create_info.format = rhi::pixel_format::rgba8un;
        tex_create_info.width = im_view.width;
        tex_create_info.height = im_view.height;
        tex_create_info.depth = 1;
        tex_create_info.usage = rhi::k_default_texture_usage;
        tex_create_info.mip_levels = 1;
        tex_create_info.array_layers = 1;
        tex_create_info.sample_count = 1;

        m_texture = m_graphics_device->create_texture(tex_create_info);
        if (!m_texture) {
            ::logger.fatal("Failed to create texture");
            exit_fail();
        }


        auto* cbuf = m_composer->create_command_list();
        cbuf->copy_buffer_to_texture(m_stage_buffer, m_texture, 0, tex_size, 0);
        m_composer->submit_command_list(cbuf);

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
        main_render_pass.color_attachments.push_back({rhi::pixel_format::rgba8un, 1, rhi::load_op::clear, rhi::store_op::dont_care, 0, {0.2f, 0.2f, 0.25f, 1.0f}});
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
        shader_binding_info.texture_bindings.push_back({0, 0, 0});
        rhi::texture_handle textures_to_binding_main[] = {m_texture};
        rhi::sampler_handle samplers_to_binding_main[] = {m_sampler};
        m_shader_binding = m_graphics_device->create_shader_binding(shader_binding_info, textures_to_binding_main, samplers_to_binding_main, {});
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
        mesh_rendering_pipeline_info.attributes.push_back({rhi::attribute_type::vec2, rhi::attribute_format::f32, false, 3});

        mesh_rendering_pipeline_info.shaders.push_back({rhi::shader_stage::vertex, "main"});
        mesh_rendering_pipeline_info.shaders.push_back({rhi::shader_stage::fragment, "main"});
        mesh_rendering_pipeline_info.depth_stencil.depth_test_enable = true;
        mesh_rendering_pipeline_info.depth_stencil.depth_write_enable = true;
        mesh_rendering_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
        mesh_rendering_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        mesh_rendering_pipeline_info.rasterizer.face = rhi::front_face::counter_clockwise;
        mesh_rendering_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        mesh_rendering_pipeline_info.topology = rhi::primitive_topology::triangles;
        mesh_rendering_pipeline_info.blend_states.push_back({false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
        mesh_rendering_pipeline_info.blend_states.push_back({false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
        mesh_rendering_pipeline_info.multisample.sample_shading_enabled = false;
        mesh_rendering_pipeline_info.multisample.sample_count = 1;
        mesh_rendering_pipeline_info.multisample.min_sample_shading = 0.0;

        rhi::shader_handle mesh_rendering_shaders[] = {mesh_rendering_vertex_shader, mesh_rendering_fragment_shader};
        m_mesh_rendering_pipeline = m_graphics_device->create_pipeline(mesh_rendering_pipeline_info, mesh_rendering_shaders);
        if (!m_mesh_rendering_pipeline) {
            ::logger.fatal("Failed to create mesh rendering pipeline");
            exit_fail();
        }


        rhi::buffer_create_info mesh_vertices_buffer_info{1024 * 1024 * 16, rhi::buffer_usage::vertex, rhi::buffer_access::gpu_only};
        auto                    mesh_vertices_buffer = m_graphics_device->create_buffer(mesh_vertices_buffer_info);
        if (!mesh_vertices_buffer) {
            ::logger.fatal("Failed to create mesh vertices buffer");
            exit_fail();
        }

        rhi::buffer_create_info mesh_indices_buffer_info{1024 * 128, rhi::buffer_usage::index, rhi::buffer_access::gpu_only};
        auto                    mesh_indices_buffer = m_graphics_device->create_buffer(mesh_indices_buffer_info);
        if (!mesh_indices_buffer) {
            ::logger.fatal("Failed to create mesh indices buffer");
            exit_fail();
        }

        rhi::geometry_create_info mesh_geometry_info;
        mesh_geometry_info.buffer_layouts.push_back({0, 0, sizeof(app::vertex_type)});
        mesh_geometry_info.attribute_bindings.push_back({0, offsetof(app::vertex_type, app::vertex_type::pos), 0, rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0});
        mesh_geometry_info.attribute_bindings.push_back({0, offsetof(app::vertex_type, app::vertex_type::normal), 0, rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 1});
        mesh_geometry_info.attribute_bindings.push_back({0, offsetof(app::vertex_type, app::vertex_type::uv_outside), 0, rhi::attribute_type::vec2, rhi::attribute_format::f32, false, 2});
        mesh_geometry_info.attribute_bindings.push_back({0, offsetof(app::vertex_type, app::vertex_type::uv_inside), 0, rhi::attribute_type::vec2, rhi::attribute_format::f32, false, 3});
        mesh_geometry_info.has_index_buffer = true;
        mesh_geometry_info.index_format = rhi::index_buffer_format::u32;

        rhi::buffer_handle buffers_to_binding[] = {mesh_vertices_buffer};
        m_mesh_geometry = m_graphics_device->create_geometry(mesh_geometry_info, buffers_to_binding, mesh_indices_buffer);
        if (!m_mesh_geometry) {
            ::logger.fatal("Failed to create mesh geometry");
            exit_fail();
        }

        auto stage_map = m_graphics_device->map_buffer(m_stage_buffer);
        memcpy(stage_map.data(), app::cube_vertices, sizeof(app::cube_vertices));
        memcpy(stage_map.data() + sizeof(app::cube_vertices), app::cube_indices, sizeof(app::cube_indices));
        m_graphics_device->unmap_buffer(m_stage_buffer);


        cbuf = m_composer->create_command_list();
        cbuf->copy_buffer(m_stage_buffer, mesh_vertices_buffer, sizeof(app::cube_vertices), 0, 0);
        cbuf->copy_buffer(m_stage_buffer, mesh_indices_buffer, sizeof(app::cube_indices), sizeof(app::cube_vertices), 0);
        m_composer->submit_command_list(cbuf);

        rhi::shader_binding_create_info mesh_shader_binding_info;
        mesh_shader_binding_info.buffer_bindings.push_back({0, 0, sizeof(frame_data), 0});
        mesh_shader_binding_info.texture_bindings.push_back({0, 0, 0});

        rhi::texture_handle mesh_textures_to_binding[] = {m_texture};
        rhi::sampler_handle mesh_samplers_to_binding[] = {m_sampler};
        rhi::buffer_handle  mesh_ubo_buffers_to_binding[] = {m_uniform_buffer};

        m_mesh_shader_binding = m_graphics_device->create_shader_binding(mesh_shader_binding_info, mesh_textures_to_binding, mesh_samplers_to_binding, mesh_ubo_buffers_to_binding);
        if (!m_mesh_shader_binding) {
            ::logger.fatal("Failed to create shader binding");
            exit_fail();
        }


        // World grid
        auto world_grid_rendering_vertex_shader = m_graphics_device->create_shader({world_grid_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto world_grid_rendering_fragment_shader = m_graphics_device->create_shader({world_grid_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info world_grid_rendering_pipeline_info;
        world_grid_rendering_pipeline_info.shaders.push_back({rhi::shader_stage::vertex, "main"});
        world_grid_rendering_pipeline_info.shaders.push_back({rhi::shader_stage::fragment, "main"});
        world_grid_rendering_pipeline_info.depth_stencil.depth_test_enable = true;
        world_grid_rendering_pipeline_info.depth_stencil.depth_write_enable = true;
        world_grid_rendering_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
        world_grid_rendering_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        world_grid_rendering_pipeline_info.rasterizer.face = rhi::front_face::counter_clockwise;
        world_grid_rendering_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        world_grid_rendering_pipeline_info.topology = rhi::primitive_topology::triangle_strip;
        world_grid_rendering_pipeline_info.blend_states.push_back({true, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
        world_grid_rendering_pipeline_info.blend_states.push_back({true, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
        world_grid_rendering_pipeline_info.multisample.sample_shading_enabled = false;
        world_grid_rendering_pipeline_info.multisample.sample_count = 1;
        world_grid_rendering_pipeline_info.multisample.min_sample_shading = 0.0;

        rhi::shader_handle world_grid_rendering_shaders[] = {world_grid_rendering_vertex_shader, world_grid_rendering_fragment_shader};
        m_world_grid_rendering_pipeline = m_graphics_device->create_pipeline(world_grid_rendering_pipeline_info, world_grid_rendering_shaders);
        if (!m_world_grid_rendering_pipeline) {
            ::logger.fatal("Failed to create world grid rendering pipeline");
            exit_fail();
        }


        rhi::shader_binding_create_info world_grid_shader_binding_info;
        world_grid_shader_binding_info.buffer_bindings.push_back({0, 0, sizeof(frame_data), 0});
        rhi::buffer_handle world_grid_ubo_buffers_to_binding[] = {m_uniform_buffer};

        m_world_grid_shader_binding = m_graphics_device->create_shader_binding(world_grid_shader_binding_info, {}, {}, world_grid_ubo_buffers_to_binding);
        if (!m_world_grid_shader_binding) {
            ::logger.fatal("Failed to create world grid shader binding");
            exit_fail();
        }
    }

    void shutdown() override
    {
        m_graphics_device = nullptr;
    }

    void process_events(app::event_queue_view events, double delta_time)
    {
        m_input_manager.on_frame_started(tavros::system::get_high_precision_system_time_us());

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
                m_current_frame_size = tavros::math::ivec2(static_cast<int32>(it.vec_info.x), static_cast<int32>(it.vec_info.y));
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

        bool need_buffer_switch = false;

        if (m_input_manager.is_key_released(tavros::system::keys::k_F1)) {
            if (m_current_buffer_output_index != 0) {
                m_current_buffer_output_index = 0;
                need_buffer_switch = true;
            }
        } else if (m_input_manager.is_key_released(tavros::system::keys::k_F2)) {
            if (m_current_buffer_output_index != 1) {
                m_current_buffer_output_index = 1;
                need_buffer_switch = true;
            }
        }

        if ((need_resize || need_buffer_switch) && m_current_frame_size.width != 0 && m_current_frame_size.height != 0) {
            m_offscreen_rt->recreate(static_cast<uint32>(m_current_frame_size.width), static_cast<uint32>(m_current_frame_size.height), 32);

            if (m_fullscreen_quad_shader_binding) {
                m_graphics_device->destroy_shader_binding(m_fullscreen_quad_shader_binding);
                m_fullscreen_quad_shader_binding = rhi::shader_binding_handle();
            }

            rhi::shader_binding_create_info fullscreen_quad_shader_binding_info;
            fullscreen_quad_shader_binding_info.texture_bindings.push_back({0, 0, 0});
            rhi::texture_handle fullscreen_quad_textures_to_binding[] = {m_offscreen_rt->get_color_attachment(m_current_buffer_output_index)};
            rhi::sampler_handle fullscreen_quad_samplers_to_binding[] = {m_sampler};

            m_fullscreen_quad_shader_binding = m_graphics_device->create_shader_binding(fullscreen_quad_shader_binding_info, fullscreen_quad_textures_to_binding, fullscreen_quad_samplers_to_binding, {});
            if (!m_fullscreen_quad_shader_binding) {
                ::logger.fatal("Failed to create fullscreen quad shader binding");
                exit_fail();
            }

            m_composer->resize(m_current_frame_size.width, m_current_frame_size.height);

            constexpr float fov_y = 60.0f * 3.14159265358979f / 180.0f; // 60 deg
            float           aspect_ratio = static_cast<float>(m_current_frame_size.width) / static_cast<float>(m_current_frame_size.height);
            m_camera.set_perspective(fov_y, aspect_ratio, 0.1f, 100.0f);
        }

        // Update camera
        auto factor = [&](tavros::system::keys key) -> float {
            return static_cast<float>(m_input_manager.key_pressed_factor(key));
        };

        // clang-format off
        tavros::math::vec3 move_delta =
            m_camera.forward() * factor(tavros::system::keys::k_W)
            - m_camera.forward() * factor(tavros::system::keys::k_S)
            + m_camera.right() * factor(tavros::system::keys::k_D)
            - m_camera.right() * factor(tavros::system::keys::k_A) 
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

            auto q_yaw = tavros::math::quat::from_axis_angle(world_up, -scaled_mouse_delta.x);
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
        m_renderer_frame_data.view = tavros::math::transpose(m_camera.get_view_matrix());
        m_renderer_frame_data.perspective_projection = tavros::math::transpose(m_camera.get_projection_matrix());
        m_renderer_frame_data.view_projection = tavros::math::transpose(m_camera.get_view_projection_matrix());
        m_renderer_frame_data.inverse_view = tavros::math::transpose(tavros::math::inverse(m_camera.get_view_matrix()));
        m_renderer_frame_data.inverse_projection = tavros::math::transpose(tavros::math::inverse(m_camera.get_projection_matrix()));
        m_renderer_frame_data.frame_width = static_cast<float>(m_current_frame_size.width);
        m_renderer_frame_data.frame_height = static_cast<float>(m_current_frame_size.height);
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
        memcpy(uniform_buffer_data_map.data(), &m_renderer_frame_data, sizeof(m_renderer_frame_data));
        m_graphics_device->unmap_buffer(m_stage_buffer);


        auto* cbuf = m_composer->create_command_list();
        m_composer->begin_frame();

        // Copy m_renderer_frame_data to shader
        cbuf->copy_buffer(m_stage_buffer, m_uniform_buffer, sizeof(m_renderer_frame_data));

        cbuf->begin_render_pass(m_offscreen_rt->render_pass(), m_offscreen_rt->framebuffer());

        // Draw cube
        cbuf->bind_pipeline(m_mesh_rendering_pipeline);
        cbuf->bind_geometry(m_mesh_geometry);
        cbuf->bind_shader_binding(m_mesh_shader_binding);
        cbuf->draw_indexed(6 * 6);

        // Draw world grid
        cbuf->bind_pipeline(m_world_grid_rendering_pipeline);
        cbuf->bind_shader_binding(m_world_grid_shader_binding);
        cbuf->draw(4);

        cbuf->end_render_pass();

        if (is_f10_released) {
            auto image_size = m_current_frame_size.width * m_current_frame_size.height * 4;
            auto depth_image_size = m_current_frame_size.width * m_current_frame_size.height;
            cbuf->copy_texture_to_buffer(m_offscreen_rt->get_color_attachment(0), m_stage_upload_buffer, 0, image_size, 0, 0);
            cbuf->copy_texture_to_buffer(m_offscreen_rt->get_color_attachment(1), m_stage_upload_buffer, 0, depth_image_size, image_size, 0);
        }

        // Draw to backbuffer
        cbuf->begin_render_pass(m_main_pass, m_composer->backbuffer());

        cbuf->bind_pipeline(m_fullscreen_quad_pipeline);
        cbuf->bind_shader_binding(m_fullscreen_quad_shader_binding);
        cbuf->draw(4);

        cbuf->end_render_pass();

        m_composer->submit_command_list(cbuf);
        m_composer->end_frame();
        m_composer->present();

        if (is_f10_released) {
            auto screenshot_pixels = m_graphics_device->map_buffer(m_stage_upload_buffer);

            // Save screenshot
            int width = static_cast<int>(m_current_frame_size.width);
            int height = static_cast<int>(m_current_frame_size.height);
            int channels = 4;
            stbi_flip_vertically_on_write(true);
            stbi_write_png("C:/Users/Piotr/Desktop/screenshots/image.png", width, height, channels, screenshot_pixels.data(), width * channels);
            stbi_write_png("C:/Users/Piotr/Desktop/screenshots/depth.png", width, height, 4, screenshot_pixels.data() + width * height * channels, width * channels);

            m_graphics_device->unmap_buffer(m_stage_upload_buffer);
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(330));
    }

private:
    tavros::core::mallocator                       m_allocator;
    tavros::core::unique_ptr<rhi::graphics_device> m_graphics_device;
    rhi::frame_composer*                           m_composer = nullptr;

    app::image_decoder m_image_decoder;

    tavros::core::unique_ptr<render_target> m_offscreen_rt;

    rhi::pipeline_handle       m_fullscreen_quad_pipeline;
    rhi::pipeline_handle       m_mesh_rendering_pipeline;
    rhi::pipeline_handle       m_world_grid_rendering_pipeline;
    rhi::render_pass_handle    m_main_pass;
    rhi::shader_binding_handle m_shader_binding;
    rhi::shader_binding_handle m_mesh_shader_binding;
    rhi::shader_binding_handle m_world_grid_shader_binding;
    rhi::shader_binding_handle m_fullscreen_quad_shader_binding;
    rhi::texture_handle        m_texture;
    rhi::buffer_handle         m_stage_buffer;
    rhi::buffer_handle         m_stage_upload_buffer;
    rhi::sampler_handle        m_sampler;
    rhi::buffer_handle         m_uniform_buffer;
    rhi::geometry_handle       m_mesh_geometry;

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

        float frame_width = 0.0f;
        float frame_height = 0.0f;

        float near_plane = 0.0f;
        float far_plane = 0.0f;
        float view_space_depth = 0.0f;
        float aspect_ratio = 0.0f;
        float fov_y = 0.0f;
    };

    frame_data m_renderer_frame_data;
};

int main()
{
    tavros::core::logger::add_consumer([](auto lvl, auto tag, auto msg) {
        printf("%s\n", msg.data());
    });

    ::logger.info("Starting TavrosEngine application...");

    auto resource_manager = tavros::core::make_shared<tavros::resources::resource_manager>();
    resource_manager->mount<tavros::resources::filesystem_provider>("C:/Users/Piotr/Desktop/Tavros/assets");
    resource_manager->mount<tavros::resources::filesystem_provider>("C:/Work/q3pp_res/baseq3");

    auto app = std::make_unique<my_app>(resource_manager);
    auto exit_code = app->run();

    ::logger.info("TavrosEngine application exited with exit code {}", exit_code);

    return exit_code;
}
#include <iostream>

#include <tavros/core/prelude.hpp>
#include <tavros/core/timer.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/math/utils/make_string.hpp>

#include <tavros/system/interfaces/window.hpp>
#include <tavros/system/interfaces/application.hpp>
#include <tavros/renderer/camera/camera.hpp>

#include <tavros/renderer/rhi/command_list.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

#include <tavros/renderer/internal/opengl/command_list_opengl.hpp>
#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

#include <tavros/core/containers/static_vector.hpp>

#include <glad/glad.h>

#include <inttypes.h>

#include <thread>

#include <tavros/core/scoped_owner.hpp>

#include <stb/stb_image.h>

#include <fstream>

namespace rhi = tavros::renderer::rhi;

const char* msaa_vertex_shader_source = R"(
#version 420 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in mat4 a_instance_model;

out vec2 v_uv;
out vec3 v_world_pos;
out vec3 v_normal;

layout (binding = 0) uniform Camera
{
    mat4 u_camera;
};

layout (binding = 1) uniform Scene
{
    mat4 u_view;       // view matrix (world->view)
    vec3 u_camera_pos; // world-space camera position
    // padding rules apply in std140 — pad on CPU side if using UBO
    vec3 u_sun_dir;    // world-space, should be normalized
    vec3 u_sun_color;  // linear RGB
    float u_sun_intensity;
    float u_ambient;
    float u_specular_power;
};

void main()
{
    v_uv = a_uv;
    vec3 pos = vec3(a_instance_model * vec4(a_pos, 1.0f)).xyz;
    v_world_pos = pos;
    v_normal = normalize(a_normal);

    gl_Position = u_camera * vec4(pos, 1.0);
}
)";

const char* msaa_fragment_shader_source = R"(
#version 420 core

layout(binding = 0) uniform sampler2D u_tex1;
layout(binding = 8) uniform samplerCube  u_skyCube;

layout (binding = 1) uniform Scene
{
    mat4 u_view;
    vec3 u_camera_pos;
    vec3 u_sun_dir;
    vec3 u_sun_color;
    float u_sun_intensity;
    float u_ambient;
    float u_specular_power;
};

in vec2 v_uv;
in vec3 v_world_pos;
in vec3 v_normal;

const float PI = 3.14159265;

// Simple fresnel-like factor for rim (optional)
float fresnel_schlick(float cosTheta, float f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

out vec4 frag_color;

void main()
{
    // fetch albedo
    vec4 full_color = texture(u_tex1, v_uv);
    vec3 albedo = full_color.rgb;
    float alpha_tex = full_color.a;

    // normalize inputs
    vec3 N = normalize(v_normal);
    vec3 L = normalize(u_sun_dir); // direction from surface toward light (sun)
    vec3 V = normalize(u_camera_pos - v_world_pos); // view direction (toward camera)
    vec3 H = normalize(L + V);

    // Sample skybox cubemap
    vec3 sky_dir = -normalize(V);
    vec3 sky_color = texture(u_skyCube, sky_dir).rgb;

    // Lambertian diffuse
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = albedo * NdotL;

    // Blinn-Phong specular (simple)
    float NdotH = max(dot(N, H), 0.0);
    float specular_strength = pow(NdotH, max(1.0, u_specular_power));
    // optionally scale specular by some factor derived from albedo brightness
    float specular_factor = specular_strength;

    // Sun direct contribution
    vec3 sun_light = u_sun_color * u_sun_intensity;

    // Sun "disk" / glow seen when looking close to sun direction:
    // measure how close view direction is to opposite of sun dir (we look toward sun when dot(V, -L) ~ 1)
    float view_sun_cos = clamp(dot(V, -L), 0.0, 1.0);
    // sharp disk + soft halo: exponent controls sharpness of disk
    float sun_disk = pow(view_sun_cos, 400.0);     // sharp core
    float sun_halo = pow(view_sun_cos, 20.0) * 0.5; // soft halo
    float sun_glow = clamp(sun_disk + sun_halo, 0.0, 10.0);

    // combine lighting:
    vec3 ambient = u_ambient * albedo;
    vec3 direct = (diffuse + specular_factor) * sun_light;

    // add sun glow as additive highlight (not multiplied by albedo)
    vec3 glow = u_sun_color * sun_glow * u_sun_intensity;

    vec3 color = ambient + direct + glow;

    // simple tonemapping (Reinhard) and gamma correction
    color = color / (color + vec3(1.0));
    // gamma to sRGB
    color = pow(color, vec3(1.0/2.2));

    float sky_factor = 0.0;
    color = mix(color, sky_color, sky_factor);
    frag_color = vec4(color, alpha_tex);
}
)";

const char* fullscreen_quad_vertex_shader_source = R"(
#version 420 core

const vec2 quadVerts[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);
out vec2 texCoord;

void main()
{
    vec2 pos = quadVerts[gl_VertexID];
    texCoord = (pos + 1.0) * 0.5; // [-1..1] -> [0..1]
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

const char* fullscreen_quad_fragment_shader_source = R"(
#version 420 core

in vec2 texCoord;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D uTex;
layout(binding = 1) uniform sampler3D uLut;

void main()
{
    vec3 color = texture(uTex, texCoord).rgb;
    FragColor = texture(uLut, color);
    //FragColor = vec4(color, 1.0f);
}
)";


uint8* load_pixels_from_file(const char* filename, int32& w, int32& h, int32& c)
{
    return static_cast<uint8*>(stbi_load(filename, &w, &h, &c, STBI_rgb_alpha));
}

void free_pixels(uint8* p)
{
    stbi_image_free(p);
}

void update_camera(const bool* keys, tavros::math::vec2 mouse_delta, float elapsed, float aspect_ratio, tavros::renderer::camera& cam)
{
    tavros::math::vec3 dir;
    constexpr float    k_speed_factor = 2.0f;

    if (elapsed > 1.0f) {
        elapsed = 1.0f;
    }

    if (keys[(uint8) tavros::system::keys::k_W]) {
        dir += cam.forward();
    }

    if (keys[(uint8) tavros::system::keys::k_S]) {
        dir -= cam.forward();
    }

    if (keys[(uint8) tavros::system::keys::k_D]) {
        dir += cam.right();
    }

    if (keys[(uint8) tavros::system::keys::k_A]) {
        dir -= cam.right();
    }

    if (keys[(uint8) tavros::system::keys::k_space]) {
        dir += cam.up();
    }

    if (keys[(uint8) tavros::system::keys::k_C]) {
        dir -= cam.up();
    }

    if (tavros::math::squared_length(dir) > 0.0f) {
        auto norm = tavros::math::normalize(dir);
        cam.move(norm * elapsed * k_speed_factor);
    }

    if (tavros::math::squared_length(mouse_delta) > 0.0f) {
        auto m = (mouse_delta / 5.0f) * elapsed;

        auto q_pitch = tavros::math::quat::from_axis_angle(cam.right(), -m.y);
        auto q_yaw = tavros::math::quat::from_axis_angle(tavros::math::vec3{0.0f, 1.0f, 0.0f}, -m.x);
        auto rotation = tavros::math::normalize(q_yaw * q_pitch);

        cam.set_orientation(rotation * cam.forward(), rotation * cam.up());
    }

    cam.set_perspective(3.14159265358979f / 3.0f, aspect_ratio, 0.1f, 1000.0f);
}

constexpr auto MAX_QPATH = 64;


#define MD3_IDENT   (('3' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define MD3_VERSION 15


/*
** md3Surface_t
**
** CHUNK            SIZE
** header            sizeof( md3Surface_t )
** shaders            sizeof( md3Shader_t ) * numShaders
** triangles[0]        sizeof( md3Triangle_t ) * numTriangles
** st                sizeof( md3St_t ) * numVerts
** XyzNormals        sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/


struct md3_header_t
{
    int32 ident;           // "IDP3"
    int32 version;
    char  name[MAX_QPATH]; // model name
    int32 flags;
    int32 num_frames;
    int32 num_tags;
    int32 num_surfaces;
    int32 num_skins;
    int32 ofs_frames;   // offset for first frame
    int32 ofs_tags;     // numFrames * numTags
    int32 ofs_surfaces; // first surface, others follow
    int32 ofs_end;      // end of file
};

struct md3_surface_t
{
    int32 ident;           //
    char  name[MAX_QPATH]; // polyset name
    int32 flags;
    int32 num_frames;      // all surfaces in a model should have the same
    int32 num_shaders;     // all surfaces in a model should have the same
    int32 num_verts;
    int32 num_triangles;
    int32 ofs_triangles;
    int32 ofs_shaders;     // offset from start of md3Surface_t
    int32 ofs_st;          // texture coords are common for all frames
    int32 ofs_xyz_normals; // numVerts * numFrames
    int32 ofs_end;         // next surface follows
};

struct md3_triangle_t
{
    int32 indices[3];
};

struct md3_shader_t
{
    char  name[MAX_QPATH];
    int32 shader_index; // for in-game use
};

struct md3_vertex_t
{
    int16 xyz[3];
    int16 normal;
};

struct md3_frame_t
{
    tavros::math::vec3 bounds[2];
    tavros::math::vec3 local_origin;
    float              radius;
    char               name[16];
};

struct md3_tag_t
{
    char               name[MAX_QPATH]; // tag name
    tavros::math::vec3 origin;
    tavros::math::vec3 axis[3];
};


struct texture_t
{
    std::string name;
};

struct surface_t
{
    tavros::core::vector<uint32>    indices;
    tavros::core::vector<texture_t> textures;
};

struct model_t
{
    tavros::core::vector<tavros::math::vec3> xyz;
    tavros::core::vector<tavros::math::vec3> norm;
    tavros::core::vector<tavros::math::vec2> uv;
    tavros::core::vector<surface_t>          surfaces;
};


tavros::math::vec3 decode_normal(uint16 latlng)
{
    float lat = (latlng & 0xFF) * (2.0f * 3.14159265358979f / 255.0f);
    float lng = ((latlng >> 8) & 0xFF) * (2.0f * 3.14159265358979f / 255.0f);

    float x = std::cos(lng) * std::sin(lat);
    float y = std::sin(lng) * std::sin(lat);
    float z = std::cos(lat);

    return {x, y, z};
}


bool load_md3(std::string path, model_t* model)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        tavros::core::logger::print(tavros::core::severity_level::warning, "R_LoadMD3: couldn't read %s", path.c_str());
        return false;
    }

    md3_header_t header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.ident != MD3_IDENT || header.version != MD3_VERSION) {
        tavros::core::logger::print(tavros::core::severity_level::warning, "R_LoadMD3: %s has wrong version", path.c_str());
        return false;
    }

    file.seekg(header.ofs_surfaces, std::ios::beg);

    for (int32 s = 0; s < header.num_surfaces; ++s) {
        std::streampos surf_pos = file.tellg();

        md3_surface_t surf{};
        file.read(reinterpret_cast<char*>(&surf), sizeof(surf));

        surface_t surface{};
        // Read shaders
        file.seekg(surf_pos + std::streamoff(surf.ofs_shaders), std::ios::beg);
        for (int32 sh = 0; sh < surf.num_shaders; ++sh) {
            md3_shader_t shader{};
            file.read(reinterpret_cast<char*>(&shader), sizeof(shader));
            texture_t texture{};
            texture.name = shader.name;
            surface.textures.push_back(texture);
        }

        // Read triangles
        file.seekg(surf_pos + std::streamoff(surf.ofs_triangles), std::ios::beg);
        for (int32 t = 0; t < surf.num_triangles; ++t) {
            md3_triangle_t triangle{};
            file.read(reinterpret_cast<char*>(&triangle), sizeof(triangle));
            surface.indices.push_back(triangle.indices[0]);
            surface.indices.push_back(triangle.indices[1]);
            surface.indices.push_back(triangle.indices[2]);
        }

        // Read vertices
        file.seekg(surf_pos + std::streamoff(surf.ofs_xyz_normals), std::ios::beg);
        for (int32 v = 0; v < surf.num_verts * surf.num_frames; ++v) {
            md3_vertex_t md3_vert{};
            file.read(reinterpret_cast<char*>(&md3_vert), sizeof(md3_vert));

            model->xyz.push_back(tavros::math::vec3(md3_vert.xyz[0] / 64.0f, md3_vert.xyz[1] / 64.0f, md3_vert.xyz[2] / 64.0f) / 10.0f);
            model->norm.push_back(decode_normal(md3_vert.normal)); // Placeholder normal
        }

        // Read texture coordinates
        file.seekg(surf_pos + std::streamoff(surf.ofs_st), std::ios::beg);
        for (int32 v = 0; v < surf.num_verts; ++v) {
            struct
            {
                float u, v;
            } st{};
            file.read(reinterpret_cast<char*>(&st), sizeof(st));
            model->uv.push_back(tavros::math::vec2(st.u, st.v));
        }
        model->surfaces.push_back(surface);
        // Move to the next surface
        file.seekg(surf_pos + std::streamoff(surf.ofs_end), std::ios::beg);
    }

    return true;
}

class application_example
{
public:
    application_example()
    {
    }
    ~application_example()
    {
    }
};

constexpr auto k_window_size_factor = 2.0;
constexpr auto k_initial_window_width = static_cast<int32>(1280 * k_window_size_factor);
constexpr auto k_initial_window_height = static_cast<int32>(720 * k_window_size_factor);

int main()
{
    tavros::core::logger::add_consumer([](tavros::core::severity_level lvl, tavros::core::string_view tag, tavros::core::string_view msg) {
        TAV_ASSERT(tag.data());
        // if (lvl == tavros::core::severity_level::error) {
        std::cout << msg << std::endl;
        //}
    });

    auto logger = tavros::core::logger("main");

    auto app = tavros::system::interfaces::application::create();

    auto wnd = tavros::system::interfaces::window::create("TavrosEngine");
    wnd->set_window_size(k_initial_window_width, k_initial_window_height);
    wnd->set_location(100, 100);

    wnd->show();
    wnd->set_on_close_listener([&](tavros::system::window_ptr, tavros::system::close_event_args& e) {
        app->exit();
    });

    float aspect_ratio = k_initial_window_width / (float) k_initial_window_height;
    bool  size_changed = true;

    wnd->set_on_resize_listener([&](tavros::system::window_ptr, tavros::system::size_event_args& e) {
        aspect_ratio = static_cast<float>(e.size.width) / static_cast<float>(e.size.height);
        size_changed = true;
    });

    static bool keys[256] = {false};

    wnd->set_on_key_down_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
        keys[(uint8) e.key] = true;
    });

    wnd->set_on_key_up_listener([&](tavros::system::window_ptr, tavros::system::key_event_args& e) {
        keys[(uint8) e.key] = false;
    });

    wnd->set_on_deactivate_listener([&](tavros::system::window_ptr) {
        for (auto& v : keys) {
            v = false;
        }
    });

    wnd->set_on_activate_listener([&](tavros::system::window_ptr) {
        for (auto& v : keys) {
            v = false;
        }
    });

    tavros::math::point2 mouse_delta;

    wnd->set_on_mouse_move_listener([&](tavros::system::window_ptr, tavros::system::mouse_event_args& e) {
        if (e.is_relative_move) {
            mouse_delta = e.pos;
        }
    });

    app->run();


    auto gdevice = tavros::core::make_shared<rhi::graphics_device_opengl>();

    rhi::frame_composer_info main_composer_info;
    main_composer_info.width = k_initial_window_width;
    main_composer_info.height = k_initial_window_height;
    main_composer_info.buffer_count = 3;
    main_composer_info.vsync = true;
    main_composer_info.color_attachment_format = rhi::pixel_format::rgba8un;
    main_composer_info.depth_stencil_attachment_format = rhi::pixel_format::depth24_stencil8;

    auto main_composer_handle = gdevice->create_frame_composer(main_composer_info, wnd->get_handle());

    auto* composer = gdevice->get_frame_composer_ptr(main_composer_handle);
    auto* cbuf = composer->create_command_list();

    rhi::buffer_info stage_buffer_info{1024 * 1024 * 16 /* 16 Mb */, rhi::buffer_usage::stage, rhi::buffer_access::cpu_to_gpu};
    auto             stage_buffer = gdevice->create_buffer(stage_buffer_info);


    model_t model;
    if (!load_md3("C:\\Work\\q3pp_res\\baseq3\\models\\weapons2\\plasma\\plasma.md3", &model)) {
        logger.error("Failed to load model");
        return -1;
    }

    int32 w, h, c;
    auto* pixels = load_pixels_from_file("C:\\Work\\q3pp_res\\baseq3\\models\\weapons2\\plasma\\plasma.jpg", w, h, c);
    cbuf->copy_buffer_data(stage_buffer, pixels, w * h * 4);
    free_pixels(pixels);

    rhi::texture_info tex_desc{rhi::texture_type::texture_2d, rhi::pixel_format::rgba8un, static_cast<uint32>(w), static_cast<uint32>(h), 1, rhi::k_default_texture_usage, 1, 1, 1};
    auto              tex1 = gdevice->create_texture(tex_desc);
    cbuf->copy_buffer_to_texture(stage_buffer, tex1, 0, w * h * 4);


    uint8* lut_pixels = load_pixels_from_file("C:\\Work\\img\\null_lut.png", w, h, c);
    uint8* lut_data = reinterpret_cast<uint8*>(malloc(w * h * 4));

    uint8* dst = lut_data;
    uint8* src = lut_pixels;

    constexpr int slice_size = 64;                           // размер стороны одного квадрата
    constexpr int blocks_per_row = 8;                        // 8x8 блоков
    constexpr int size = 64;                                 // сторона 3D LUT
    constexpr int channels = 4;                              // RGBA
    constexpr int total_width = slice_size * blocks_per_row; // 512
    constexpr int total_height = total_width;                // 512

    uint8* dst_p = lut_data;
    uint8* src_p = lut_pixels;

    for (int z = 0; z < size; ++z) {
        int block_x = z % blocks_per_row;
        int block_y = z / blocks_per_row;

        for (int y = 0; y < slice_size; ++y) {
            for (int x = 0; x < slice_size; ++x) {
                // координаты в 2D текстуре
                int src_x = block_x * slice_size + x;
                int src_y = block_y * slice_size + y;

                // индексы
                int src_index = (src_y * total_width + src_x) * channels;
                int dst_index = (z * size * size + y * size + x) * channels;

                // копируем пиксель
                dst_p[dst_index + 0] = src_p[src_index + 0];
                dst_p[dst_index + 1] = src_p[src_index + 1];
                dst_p[dst_index + 2] = src_p[src_index + 2];
                dst_p[dst_index + 3] = src_p[src_index + 3];
            }
        }
    }


    cbuf->copy_buffer_data(stage_buffer, lut_data, w * h * 4);
    free(lut_data);
    free_pixels(lut_pixels);

    rhi::texture_info lut_tex_info{rhi::texture_type::texture_3d, rhi::pixel_format::rgba8un, 64, 64, 64, rhi::k_default_texture_usage, 16, 1, 1};
    auto              lut_tex = gdevice->create_texture(lut_tex_info);
    cbuf->copy_buffer_to_texture(stage_buffer, lut_tex, 0, w * h * 4);


    rhi::texture_info cube_tex_info{rhi::texture_type::texture_cube, rhi::pixel_format::rgba8un, 512, 512, 1, rhi::k_default_texture_usage, 16, 6, 1};
    auto              skycube_tex = gdevice->create_texture(cube_tex_info);


    uint8* sky_pixels = load_pixels_from_file("C:\\Work\\img\\sky2.png", w, h, c);
    cbuf->copy_buffer_data(stage_buffer, sky_pixels, w * h * 4);
    free_pixels(sky_pixels);

    cbuf->copy_buffer_to_texture(stage_buffer, skycube_tex, 2, 512 * 512 * 4, 512 * 4, 512 * 4 * 4);
    cbuf->copy_buffer_to_texture(stage_buffer, skycube_tex, 1, 512 * 512 * 4, 512 * 512 * 4 * 4, 512 * 4 * 4);
    cbuf->copy_buffer_to_texture(stage_buffer, skycube_tex, 4, 512 * 512 * 4, 512 * 512 * 4 * 4 + 512 * 4, 512 * 4 * 4);
    cbuf->copy_buffer_to_texture(stage_buffer, skycube_tex, 0, 512 * 512 * 4, 512 * 512 * 4 * 4 + 512 * 4 * 2, 512 * 4 * 4);
    cbuf->copy_buffer_to_texture(stage_buffer, skycube_tex, 5, 512 * 512 * 4, 512 * 512 * 4 * 4 + 512 * 4 * 3, 512 * 4 * 4);
    cbuf->copy_buffer_to_texture(stage_buffer, skycube_tex, 3, 512 * 512 * 4, 512 * 512 * 4 * 4 * 2 + 512 * 4, 512 * 4 * 4);

    int msaa_level = 16;

    // msaa texture
    rhi::texture_info msaa_texture_desc;
    msaa_texture_desc.width = k_initial_window_width;
    msaa_texture_desc.height = k_initial_window_height;
    msaa_texture_desc.format = rhi::pixel_format::rgba8un;
    msaa_texture_desc.usage = rhi::texture_usage::render_target | rhi::texture_usage::resolve_source;
    msaa_texture_desc.sample_count = msaa_level;
    auto msaa_texture = gdevice->create_texture(msaa_texture_desc);

    // resolve target texture
    rhi::texture_info msaa_resolve_desc;
    msaa_resolve_desc.width = k_initial_window_width;
    msaa_resolve_desc.height = k_initial_window_height;
    msaa_resolve_desc.format = rhi::pixel_format::rgba8un;
    msaa_resolve_desc.usage = rhi::texture_usage::sampled | rhi::texture_usage::resolve_destination;
    msaa_resolve_desc.sample_count = 1;
    auto msaa_resolve_texture = gdevice->create_texture(msaa_resolve_desc);

    // depth/stencil texture
    rhi::texture_info msaa_depth_stencil_desc;
    msaa_depth_stencil_desc.width = k_initial_window_width;
    msaa_depth_stencil_desc.height = k_initial_window_height;
    msaa_depth_stencil_desc.format = rhi::pixel_format::depth24_stencil8;
    msaa_depth_stencil_desc.usage = rhi::texture_usage::depth_stencil_target;
    msaa_depth_stencil_desc.sample_count = msaa_level;
    auto msaa_depth_stencil_texture = gdevice->create_texture(msaa_depth_stencil_desc);

    // msaa framebuffer
    rhi::framebuffer_info msaa_framebuffer_info;
    msaa_framebuffer_info.width = k_initial_window_width;
    msaa_framebuffer_info.height = k_initial_window_height;
    msaa_framebuffer_info.color_attachment_formats.push_back(rhi::pixel_format::rgba8un);
    msaa_framebuffer_info.depth_stencil_attachment_format = rhi::pixel_format::depth24_stencil8;
    msaa_framebuffer_info.sample_count = msaa_level;
    rhi::texture_handle msaa_attachments[] = {msaa_texture};
    auto                msaa_framebuffer = gdevice->create_framebuffer(msaa_framebuffer_info, msaa_attachments, msaa_depth_stencil_texture);


    rhi::render_pass_info msaa_render_pass;
    msaa_render_pass.color_attachments.push_back({rhi::pixel_format::rgba8un, static_cast<uint32>(msaa_level), rhi::load_op::clear, rhi::store_op::resolve, 0, {0.1f, 0.1f, 0.1f, 1.0f}});
    msaa_render_pass.depth_stencil_attachment = {rhi::pixel_format::depth24_stencil8, rhi::load_op::clear, rhi::store_op::dont_care, 1.0f, rhi::load_op::clear, rhi::store_op::dont_care, 0};
    rhi::texture_handle msaa_resolve_attachments[] = {msaa_resolve_texture};
    auto                msaa_pass = gdevice->create_render_pass(msaa_render_pass, msaa_resolve_attachments);

    rhi::render_pass_info main_render_pass;
    main_render_pass.color_attachments.push_back({rhi::pixel_format::rgba8un, 1, rhi::load_op::clear, rhi::store_op::dont_care, 0, {0.1f, 0.1f, 0.4f, 1.0f}});
    main_render_pass.depth_stencil_attachment = {rhi::pixel_format::depth24_stencil8, rhi::load_op::dont_care, rhi::store_op::dont_care, 1.0f, rhi::load_op::dont_care, rhi::store_op::dont_care, 0};
    auto main_pass = gdevice->create_render_pass(main_render_pass);


    rhi::sampler_info sampler_info;
    sampler_info.filter.mipmap_filter = rhi::mipmap_filter_mode::off;
    sampler_info.filter.min_filter = rhi::filter_mode::linear;
    sampler_info.filter.mag_filter = rhi::filter_mode::linear;

    auto sampler1 = gdevice->create_sampler(sampler_info);

    rhi::sampler_info sampler_lut_info;
    sampler_lut_info.filter.mipmap_filter = rhi::mipmap_filter_mode::linear;
    sampler_lut_info.filter.min_filter = rhi::filter_mode::linear;
    sampler_lut_info.filter.mag_filter = rhi::filter_mode::linear;
    sampler_lut_info.wrap_mode.wrap_r = rhi::wrap_mode::clamp_to_edge;
    sampler_lut_info.wrap_mode.wrap_s = rhi::wrap_mode::clamp_to_edge;
    sampler_lut_info.wrap_mode.wrap_t = rhi::wrap_mode::clamp_to_edge;

    auto sampler_lut = gdevice->create_sampler(sampler_lut_info);

    rhi::sampler_info sampler_sky_info;
    sampler_sky_info.filter.mipmap_filter = rhi::mipmap_filter_mode::linear;
    sampler_sky_info.filter.min_filter = rhi::filter_mode::linear;
    sampler_sky_info.filter.mag_filter = rhi::filter_mode::linear;
    sampler_sky_info.wrap_mode.wrap_r = rhi::wrap_mode::clamp_to_edge;
    sampler_sky_info.wrap_mode.wrap_s = rhi::wrap_mode::clamp_to_edge;
    sampler_sky_info.wrap_mode.wrap_t = rhi::wrap_mode::clamp_to_edge;

    auto sampler_sky = gdevice->create_sampler(sampler_sky_info);


    auto msaa_vertex_shader = gdevice->create_shader({msaa_vertex_shader_source, rhi::shader_stage::vertex, "main"});
    auto msaa_fragment_shader = gdevice->create_shader({msaa_fragment_shader_source, rhi::shader_stage::fragment, "main"});


    rhi::pipeline_info msaa_pipeline_info;
    msaa_pipeline_info.shaders.push_back({rhi::shader_stage::vertex, "main"});
    msaa_pipeline_info.shaders.push_back({rhi::shader_stage::fragment, "main"});

    msaa_pipeline_info.attributes.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0});
    msaa_pipeline_info.attributes.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 1});
    msaa_pipeline_info.attributes.push_back({rhi::attribute_type::vec2, rhi::attribute_format::f32, false, 2});
    msaa_pipeline_info.attributes.push_back({rhi::attribute_type::mat4, rhi::attribute_format::f32, false, 3});
    msaa_pipeline_info.blend_states.push_back({true, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
    msaa_pipeline_info.depth_stencil.depth_test_enable = true;
    msaa_pipeline_info.depth_stencil.depth_write_enable = true;
    msaa_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
    msaa_pipeline_info.rasterizer.cull = rhi::cull_face::back;
    msaa_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
    msaa_pipeline_info.topology = rhi::primitive_topology::triangles;
    msaa_pipeline_info.multisample.sample_shading_enabled = true;
    msaa_pipeline_info.multisample.sample_count = msaa_level;
    msaa_pipeline_info.multisample.min_sample_shading = 0.0;

    rhi::shader_handle msaa_shaders[] = {msaa_vertex_shader, msaa_fragment_shader};
    auto               msaa_pipeline = gdevice->create_pipeline(msaa_pipeline_info, msaa_shaders);


    auto fullscreen_quad_vertex_shader = gdevice->create_shader({fullscreen_quad_vertex_shader_source, rhi::shader_stage::vertex, "main"});
    auto fullscreen_quad_fragment_shader = gdevice->create_shader({fullscreen_quad_fragment_shader_source, rhi::shader_stage::fragment, "main"});

    rhi::pipeline_info main_pipeline_info;
    main_pipeline_info.shaders.push_back({rhi::shader_stage::vertex, "main"});
    main_pipeline_info.shaders.push_back({rhi::shader_stage::fragment, "main"});
    main_pipeline_info.depth_stencil.depth_test_enable = false;
    main_pipeline_info.depth_stencil.depth_write_enable = false;
    main_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
    main_pipeline_info.rasterizer.cull = rhi::cull_face::off;
    main_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
    main_pipeline_info.topology = rhi::primitive_topology::triangle_strip;
    main_pipeline_info.blend_states.push_back({false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});


    rhi::shader_handle fullscreen_quad_shaders[] = {fullscreen_quad_vertex_shader, fullscreen_quad_fragment_shader};
    auto               main_pipeline = gdevice->create_pipeline(main_pipeline_info, fullscreen_quad_shaders);

    // Copy XYZ
    uint32 xyz_offset = 0;
    uint32 xyz_size = model.xyz.size() * sizeof(tavros::math::vec3);
    cbuf->copy_buffer_data(stage_buffer, reinterpret_cast<void*>(model.xyz.data()), xyz_size, xyz_offset);

    // Copy Normals
    uint32 norm_offset = xyz_offset + xyz_size;
    uint32 norm_size = model.norm.size() * sizeof(tavros::math::vec3);
    cbuf->copy_buffer_data(stage_buffer, reinterpret_cast<void*>(model.norm.data()), norm_size, norm_offset);

    // Copy UV
    uint32 uv_offset = norm_offset + norm_size;
    uint32 uv_size = model.uv.size() * sizeof(tavros::math::vec2);
    cbuf->copy_buffer_data(stage_buffer, reinterpret_cast<void*>(model.uv.data()), uv_size, uv_offset);

    // Copy Indices
    uint64 indices_offset = uv_offset + uv_size;
    uint64 indices_size = model.surfaces[0].indices.size() * sizeof(uint32);
    cbuf->copy_buffer_data(stage_buffer, reinterpret_cast<void*>(model.surfaces[0].indices.data()), model.surfaces[0].indices.size() * sizeof(uint32), indices_offset);


    // Make vertices buffer
    rhi::buffer_info xyz_normal_uv_info{1024 * 1024 * 16, rhi::buffer_usage::vertex, rhi::buffer_access::gpu_only};
    auto             buffer_xyz_normal_uv = gdevice->create_buffer(xyz_normal_uv_info);

    cbuf->copy_buffer(stage_buffer, buffer_xyz_normal_uv, xyz_size + norm_size + uv_size, 0, 0);


    rhi::buffer_info indices_desc{1024 * 128, rhi::buffer_usage::index, rhi::buffer_access::gpu_only};
    auto             buffer_indices = gdevice->create_buffer(indices_desc);

    cbuf->copy_buffer(stage_buffer, buffer_indices, indices_size, indices_offset, 0);

    auto                                     instance_number = 100;
    tavros::core::vector<tavros::math::mat4> instance_data;
    instance_data.reserve(instance_number);
    instance_data.push_back(tavros::math::mat4(1.0f));
    for (auto i = 1; i < instance_number; i++) {
        tavros::math::mat4 m(1.0f);
        m[3][0] = rand() % 30 - 15.0f;
        m[3][1] = rand() % 30 - 15.0f;
        m[3][2] = rand() % 30 - 15.0f;
        auto q = tavros::math::quat::from_axis_angle({(float) ((rand() % 100) - 50), (float) ((rand() % 100) - 50), (float) ((rand() % 100) - 50)}, ((rand() % 1000) - 500) / 250.0f);
        auto m2 = tavros::math::make_mat4(q);

        instance_data.push_back(m * m2);
    }
    cbuf->copy_buffer_data(stage_buffer, reinterpret_cast<void*>(instance_data.data()), instance_data.size() * sizeof(tavros::math::mat4));


    rhi::buffer_info instance_buffer_desc{sizeof(tavros::math::mat4) * instance_number, rhi::buffer_usage::vertex, rhi::buffer_access::gpu_only};
    auto             instance_buffer = gdevice->create_buffer(instance_buffer_desc);

    cbuf->copy_buffer(stage_buffer, instance_buffer, sizeof(tavros::math::mat4) * instance_number, 0, 0);


    rhi::geometry_info gbi;
    gbi.buffer_layouts.push_back({0, xyz_offset, 4 * 3});
    gbi.buffer_layouts.push_back({0, norm_offset, 4 * 3});
    gbi.buffer_layouts.push_back({0, uv_offset, 4 * 2});
    gbi.buffer_layouts.push_back({1, 0, sizeof(tavros::math::mat4)});

    gbi.attribute_bindings.push_back({0, 0, 0, rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0});
    gbi.attribute_bindings.push_back({1, 0, 0, rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 1});
    gbi.attribute_bindings.push_back({2, 0, 0, rhi::attribute_type::vec2, rhi::attribute_format::f32, false, 2});
    gbi.attribute_bindings.push_back({3, 0, 1, rhi::attribute_type::mat4, rhi::attribute_format::f32, false, 3});

    gbi.has_index_buffer = true;
    gbi.index_format = rhi::index_buffer_format::u32;

    rhi::buffer_handle buffers_to_binding[] = {buffer_xyz_normal_uv, instance_buffer};
    auto               geometry1 = gdevice->create_geometry(gbi, buffers_to_binding, buffer_indices);


    composer->submit_command_list(cbuf);


    auto cam = tavros::renderer::camera({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0});

    tavros::core::timer tm;

    float sun_angle = 0.0f;


    rhi::buffer_info uniform_buffer_desc{1024, rhi::buffer_usage::uniform, rhi::buffer_access::gpu_only};
    auto             uniform_buffer = gdevice->create_buffer(uniform_buffer_desc);


    struct camera_shader_t
    {
        tavros::math::mat4 u_camera;
    };

    struct scene_shader_t
    {
        tavros::math::mat4 u_view;
        tavros::math::vec3 u_camera_pos;
        float              pad1 = 0.0f;
        tavros::math::vec3 u_sun_dir;
        float              padg2 = 0.0f;
        tavros::math::vec3 u_sun_color;
        float              u_sun_intensity = 20.0f;
        float              u_ambient = 0.1f;
        float              u_specular_power = 16.0f;
        float              pad3 = 0.0f;
    };


    rhi::shader_binding_info shader_binding_info;
    shader_binding_info.buffer_bindings.push_back({0, 0, sizeof(camera_shader_t), 0});
    shader_binding_info.buffer_bindings.push_back({0, 256, sizeof(scene_shader_t), 1});
    shader_binding_info.texture_bindings.push_back({0, 0, 0});
    shader_binding_info.texture_bindings.push_back({1, 1, 8});

    rhi::texture_handle textures_to_binding[] = {tex1, skycube_tex};
    rhi::sampler_handle samplers_to_binding[] = {sampler1, sampler_sky};
    rhi::buffer_handle  ubo_buffers_to_binding[] = {uniform_buffer};

    auto shader_binding = gdevice->create_shader_binding(shader_binding_info, textures_to_binding, samplers_to_binding, ubo_buffers_to_binding);


    rhi::shader_binding_info fullstreen_shader_binding_info;
    fullstreen_shader_binding_info.texture_bindings.push_back({0, 0, 0});
    fullstreen_shader_binding_info.texture_bindings.push_back({1, 1, 1});

    rhi::texture_handle textures_to_binding_main[] = {msaa_resolve_texture, lut_tex};
    rhi::sampler_handle samplers_to_binding_main[] = {sampler1, sampler_lut};

    auto fullstreen_shader_binding = gdevice->create_shader_binding(fullstreen_shader_binding_info, textures_to_binding_main, samplers_to_binding_main, {});


    while (app->is_runing()) {
        app->poll_events();


        float elapsed = tm.elapsed<std::chrono::microseconds>() / 1000000.0f;
        tm.start();

        update_camera(keys, mouse_delta, elapsed, aspect_ratio, cam);
        mouse_delta = tavros::math::vec2();

        auto cam_mat = cam.get_view_projection_matrix();
        cam_mat = tavros::math::transpose(cam_mat);


        camera_shader_t camera_shader_data;
        scene_shader_t  scene_shader_data;

        camera_shader_data.u_camera = cam_mat;

        scene_shader_data.u_view = tavros::math::transpose(cam.get_view_matrix());
        scene_shader_data.u_camera_pos = cam.position();
        scene_shader_data.u_sun_dir = tavros::math::normalize(tavros::math::vec3(std::cos(sun_angle), std::sin(sun_angle), -0.5f));
        scene_shader_data.u_sun_color = tavros::math::vec3(1.0f, 0.9f, 0.5f);
        scene_shader_data.u_sun_intensity = 1.0f;
        scene_shader_data.u_ambient = 0.1f;
        scene_shader_data.u_specular_power = 256.0f;


        auto align_in_ubo = (sizeof(camera_shader_t) + 255) & ~255;


        auto* composer = gdevice->get_frame_composer_ptr(main_composer_handle);

        if (size_changed) {
            composer->resize(wnd->get_client_size().width, wnd->get_client_size().height);
            size_changed = false;
        }

        auto* cbuf = composer->create_command_list();
        composer->begin_frame();

        cbuf->copy_buffer_data(stage_buffer, &camera_shader_data, sizeof(camera_shader_t), 0);
        cbuf->copy_buffer_data(stage_buffer, &scene_shader_data, sizeof(scene_shader_t), align_in_ubo);
        cbuf->copy_buffer(stage_buffer, uniform_buffer, 512, 0, 0);


        cbuf->begin_render_pass(msaa_pass, msaa_framebuffer);

        cbuf->bind_pipeline(msaa_pipeline);

        cbuf->bind_geometry(geometry1);

        cbuf->bind_shader_binding(shader_binding);

        cbuf->draw_indexed(model.surfaces[0].indices.size(), 0, 0, instance_number);

        cbuf->end_render_pass();


        cbuf->begin_render_pass(main_pass, composer->backbuffer());

        cbuf->bind_pipeline(main_pipeline);

        cbuf->bind_shader_binding(fullstreen_shader_binding);

        cbuf->draw(4);

        cbuf->end_render_pass();

        composer->submit_command_list(cbuf);
        composer->end_frame();
        composer->present();

        sun_angle += elapsed * 0.5f;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

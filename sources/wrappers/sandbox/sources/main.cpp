#include <iostream>

#include <tavros/core/prelude.hpp>
#include <tavros/core/timer.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/math/utils/make_string.hpp>

#include <tavros/system/interfaces/window.hpp>
#include <tavros/system/interfaces/application.hpp>
#include <tavros/renderer/interfaces/gl_context.hpp>
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

#include <tavros/renderer/rhi/geometry_binding_desc.hpp>

#include <stb/stb_image.h>

#include <fstream>

// clang-format off
float cube_vertices[] = {
    // X      Y      Z    pad      U      V    pad    pad
    // Front face (Z+)
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, // bottom-left
     0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, // bottom-right
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f, // top-right

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, // bottom-left
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f, // top-right
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, // top-left

    // Back face (Z-)
    -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, // bottom-right
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, // bottom-left
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, // top-left

    -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, // bottom-right
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f, // top-left
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f, // top-right

    // Left face (X-)
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    // Right face (X+)
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,

     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    // Top face (Y+)
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,

    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    // Bottom face (Y-)
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
};

float cube_colors[] =
{
    // Front face (red)
    1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f,

    1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f,

    // Back face (green)
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,

    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,

    // Left face (blue)
    0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,

    0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f,

    // Right face (yellow)
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,

    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f,

    // Top face (purple)
    0.7f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, 1.0f, 1.0f,

    0.7f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, 1.0f, 1.0f,
    0.7f, 0.0f, 1.0f, 1.0f,

    // Bottom face (white blue)
    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,

    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f, 1.0f,
};

uint16 cube_indices[] =
{
     0,  1,  2,  3,  4,  5, // Front face
     6,  7,  8,  9, 10, 11, // Back face
    12, 13, 14, 15, 16, 17, // Left face
    18, 19, 20, 21, 22, 23, // Right face
    24, 25, 26, 27, 28, 29, // Top face
    30, 31, 32, 33, 34, 35, // Bottom face
};

// clang-format on

const char* msaa_vertex_shader_source = R"(
#version 420 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

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
    v_world_pos = a_pos;
    v_normal = normalize(a_normal);

    gl_Position = u_camera * vec4(a_pos, 1.0);
}
)";

const char* msaa_fragment_shader_source = R"(
#version 420 core

layout(binding = 0) uniform sampler2D u_tex1;

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
    vec3 albedo = texture(u_tex1, v_uv).rgb;

    // normalize inputs
    vec3 N = normalize(v_normal);
    vec3 L = normalize(u_sun_dir); // direction from surface toward light (sun)
    vec3 V = normalize(u_camera_pos - v_world_pos); // view direction (toward camera)
    vec3 H = normalize(L + V);

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

    frag_color = vec4(color, 1.0);
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

void main()
{
    FragColor = texture(uTex, texCoord);
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


    auto gdevice = tavros::core::make_shared<tavros::renderer::graphics_device_opengl>();

    tavros::renderer::frame_composer_desc main_composer_desc;
    main_composer_desc.width = k_initial_window_width;
    main_composer_desc.height = k_initial_window_height;
    main_composer_desc.buffer_count = 3;
    main_composer_desc.vsync = true;
    main_composer_desc.color_attachment_format = tavros::renderer::pixel_format::rgba8un;
    main_composer_desc.depth_stencil_attachment_format = tavros::renderer::pixel_format::depth24_stencil8;

    auto main_composer_handle = gdevice->create_frame_composer(main_composer_desc, wnd->get_handle());

    tavros::renderer::texture_desc tex_desc;

    model_t model;
    if (!load_md3("C:\\Work\\q3pp_res\\baseq3\\models\\weapons2\\plasma\\plasma.md3", &model)) {
        logger.error("Failed to load model");
        return -1;
    }

    int32 w, h, c;
    auto* pixels = load_pixels_from_file("C:\\Work\\q3pp_res\\baseq3\\models\\weapons2\\plasma\\plasma.jpg", w, h, c);
    tex_desc.width = w;
    tex_desc.height = h;
    tex_desc.mip_levels = 1;

    auto tex1 = gdevice->create_texture(tex_desc, pixels);
    free_pixels(pixels);

    int msaa_level = 16;

    // msaa texture
    tavros::renderer::texture_desc msaa_texture_desc;
    msaa_texture_desc.width = k_initial_window_width;
    msaa_texture_desc.height = k_initial_window_height;
    msaa_texture_desc.format = tavros::renderer::pixel_format::rgba8un;
    msaa_texture_desc.usage = tavros::renderer::texture_usage::render_target | tavros::renderer::texture_usage::resolve_source;
    msaa_texture_desc.sample_count = msaa_level;
    auto msaa_texture = gdevice->create_texture(msaa_texture_desc);

    // resolve target texture
    tavros::renderer::texture_desc msaa_resolve_desc;
    msaa_resolve_desc.width = k_initial_window_width;
    msaa_resolve_desc.height = k_initial_window_height;
    msaa_resolve_desc.format = tavros::renderer::pixel_format::rgba8un;
    msaa_resolve_desc.usage = tavros::renderer::texture_usage::sampled | tavros::renderer::texture_usage::resolve_destination;
    msaa_resolve_desc.sample_count = 1;
    auto msaa_resolve_texture = gdevice->create_texture(msaa_resolve_desc);

    // depth/stencil texture
    tavros::renderer::texture_desc msaa_depth_stencil_desc;
    msaa_depth_stencil_desc.width = k_initial_window_width;
    msaa_depth_stencil_desc.height = k_initial_window_height;
    msaa_depth_stencil_desc.format = tavros::renderer::pixel_format::depth24_stencil8;
    msaa_depth_stencil_desc.usage = tavros::renderer::texture_usage::depth_stencil_target;
    msaa_depth_stencil_desc.sample_count = msaa_level;
    auto msaa_depth_stencil_texture = gdevice->create_texture(msaa_depth_stencil_desc);

    // msaa framebuffer
    tavros::renderer::framebuffer_desc msaa_framebuffer_desc;
    msaa_framebuffer_desc.width = k_initial_window_width;
    msaa_framebuffer_desc.height = k_initial_window_height;
    msaa_framebuffer_desc.color_attachment_formats.push_back(tavros::renderer::pixel_format::rgba8un);
    msaa_framebuffer_desc.color_attachment_formats.push_back(tavros::renderer::pixel_format::rgba8un);
    msaa_framebuffer_desc.depth_stencil_attachment_format = tavros::renderer::pixel_format::depth24_stencil8;
    msaa_framebuffer_desc.sample_count = msaa_level;
    tavros::renderer::texture_handle msaa_attachments[] = {msaa_texture, msaa_resolve_texture};
    auto                             msaa_framebuffer = gdevice->create_framebuffer(msaa_framebuffer_desc, msaa_attachments, msaa_depth_stencil_texture);


    tavros::renderer::render_pass_desc msaa_render_pass;
    msaa_render_pass.color_attachments.push_back({tavros::renderer::pixel_format::rgba8un, tavros::renderer::load_op::clear, tavros::renderer::store_op::resolve, 1, {0.1f, 0.1f, 0.1f, 1.0f}});
    msaa_render_pass.color_attachments.push_back({tavros::renderer::pixel_format::rgba8un, tavros::renderer::load_op::dont_care, tavros::renderer::store_op::store, 0, {0.0f, 0.0f, 0.0f, 0.0f}});
    msaa_render_pass.depth_stencil_attachment = {tavros::renderer::pixel_format::depth24_stencil8, tavros::renderer::load_op::clear, tavros::renderer::store_op::dont_care, 1.0f, tavros::renderer::load_op::clear, tavros::renderer::store_op::dont_care, 0};
    auto msaa_pass = gdevice->create_render_pass(msaa_render_pass);

    tavros::renderer::render_pass_desc main_render_pass;
    main_render_pass.color_attachments.push_back({tavros::renderer::pixel_format::rgba8un, tavros::renderer::load_op::clear, tavros::renderer::store_op::dont_care, 0, {0.1f, 0.1f, 0.4f, 1.0f}});
    main_render_pass.depth_stencil_attachment = {tavros::renderer::pixel_format::depth24_stencil8, tavros::renderer::load_op::dont_care, tavros::renderer::store_op::dont_care, 1.0f, tavros::renderer::load_op::dont_care, tavros::renderer::store_op::dont_care, 0};
    auto main_pass = gdevice->create_render_pass(main_render_pass);


    tavros::renderer::sampler_desc samler_desc;
    samler_desc.filter.mipmap_filter = tavros::renderer::mipmap_filter_mode::off;
    samler_desc.filter.min_filter = tavros::renderer::filter_mode::linear;
    samler_desc.filter.mag_filter = tavros::renderer::filter_mode::linear;

    auto sampler1 = gdevice->create_sampler(samler_desc);


    tavros::renderer::pipeline_desc msaa_pipeline_desc;
    msaa_pipeline_desc.shaders.fragment_source = msaa_fragment_shader_source;
    msaa_pipeline_desc.shaders.vertex_source = msaa_vertex_shader_source;
    msaa_pipeline_desc.depth_stencil.depth_test_enable = true;
    msaa_pipeline_desc.depth_stencil.depth_write_enable = true;
    msaa_pipeline_desc.depth_stencil.depth_compare = tavros::renderer::compare_op::less;
    msaa_pipeline_desc.rasterizer.cull = tavros::renderer::cull_face::off;
    msaa_pipeline_desc.rasterizer.polygon = tavros::renderer::polygon_mode::fill;
    msaa_pipeline_desc.topology = tavros::renderer::primitive_topology::triangles;

    auto msaa_pipeline = gdevice->create_pipeline(msaa_pipeline_desc);


    tavros::renderer::pipeline_desc main_pipeline_desc;
    main_pipeline_desc.shaders.fragment_source = fullscreen_quad_fragment_shader_source;
    main_pipeline_desc.shaders.vertex_source = fullscreen_quad_vertex_shader_source;
    main_pipeline_desc.depth_stencil.depth_test_enable = false;
    main_pipeline_desc.depth_stencil.depth_write_enable = false;
    main_pipeline_desc.depth_stencil.depth_compare = tavros::renderer::compare_op::less;
    main_pipeline_desc.rasterizer.cull = tavros::renderer::cull_face::off;
    main_pipeline_desc.rasterizer.polygon = tavros::renderer::polygon_mode::fill;
    main_pipeline_desc.topology = tavros::renderer::primitive_topology::triangle_strip;

    auto main_pipeline = gdevice->create_pipeline(main_pipeline_desc);

    tavros::renderer::buffer_desc stage_buffer_desc;
    stage_buffer_desc.size = 1024 * 1024; // 1 Mb
    stage_buffer_desc.access = tavros::renderer::buffer_access::cpu_to_gpu;
    auto stage_buffer = gdevice->create_buffer(stage_buffer_desc);

    auto* composer = gdevice->get_frame_composer_ptr(main_composer_handle);
    auto* cbuf = composer->create_command_list();

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
    tavros::renderer::buffer_desc xyz_normal_uv_desc;
    xyz_normal_uv_desc.size = 1024 * 1024; // 1 Mb
    xyz_normal_uv_desc.usage = tavros::renderer::buffer_usage::vertex;
    xyz_normal_uv_desc.access = tavros::renderer::buffer_access::gpu_only;
    auto buffer_xyz_normal_uv = gdevice->create_buffer(xyz_normal_uv_desc);

    cbuf->copy_buffer(buffer_xyz_normal_uv, stage_buffer, xyz_size + norm_size + uv_size, 0, 0);


    tavros::renderer::buffer_desc indices_desc;
    indices_desc.size = 1024 * 128; // 128 Kb
    indices_desc.usage = tavros::renderer::buffer_usage::index;
    auto buffer_indices = gdevice->create_buffer(indices_desc);

    cbuf->copy_buffer(buffer_indices, stage_buffer, indices_size, 0, indices_offset);


    tavros::renderer::geometry_binding_desc gbd;
    gbd.buffer_layouts.push_back({0, xyz_offset, 4 * 3});
    gbd.buffer_layouts.push_back({0, norm_offset, 4 * 3});
    gbd.buffer_layouts.push_back({0, uv_offset, 4 * 2});

    gbd.attribute_bindings.push_back({0, 0, 0, 3, tavros::renderer::attribute_format::f32, false});
    gbd.attribute_bindings.push_back({1, 1, 0, 3, tavros::renderer::attribute_format::f32, false});
    gbd.attribute_bindings.push_back({2, 2, 0, 3, tavros::renderer::attribute_format::f32, false});

    gbd.has_index_buffer = true;
    gbd.index_format = tavros::renderer::index_buffer_format::u32;

    tavros::renderer::buffer_handle buffers_to_binding[] = {buffer_xyz_normal_uv};
    auto                            geometry1 = gdevice->create_geometry(gbd, buffers_to_binding, buffer_indices);


    composer->submit_command_list(cbuf);


    auto cam = tavros::renderer::camera({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0});

    tavros::core::timer tm;

    float sun_angle = 0.0f;


    tavros::renderer::buffer_desc uniform_buffer_desc{1024, tavros::renderer::buffer_usage::uniform, tavros::renderer::buffer_access::gpu_only};
    auto                          uniform_buffer = gdevice->create_buffer(uniform_buffer_desc);


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


    tavros::renderer::shader_binding_desc shader_binding_info;
    shader_binding_info.buffer_bindings.push_back({0, 0, sizeof(camera_shader_t), 0});
    shader_binding_info.buffer_bindings.push_back({0, 256, sizeof(scene_shader_t), 1});
    shader_binding_info.texture_bindings.push_back({0, 0, 0});

    tavros::renderer::texture_handle textures_to_binding[] = {tex1};
    tavros::renderer::sampler_handle samplers_to_binding[] = {sampler1};
    tavros::renderer::buffer_handle  ubo_buffers_to_binding[] = {uniform_buffer};

    auto shader_binding = gdevice->create_shader_binding(shader_binding_info, textures_to_binding, samplers_to_binding, ubo_buffers_to_binding);


    tavros::renderer::shader_binding_desc fullstreen_shader_binding_info;
    fullstreen_shader_binding_info.texture_bindings.push_back({0, 0, 0});

    tavros::renderer::texture_handle textures_to_binding_main[] = {msaa_resolve_texture};
    tavros::renderer::sampler_handle samplers_to_binding_main[] = {sampler1};

    auto fullstreen_shader_binding = gdevice->create_shader_binding(fullstreen_shader_binding_info, textures_to_binding_main, samplers_to_binding_main, {});


    while (app->is_runing()) {
        app->poll_events();


        float elapsed = tm.elapsed<std::chrono::microseconds>() / 1000000.0f;
        tm.start();

        update_camera(keys, mouse_delta, elapsed, aspect_ratio, cam);
        mouse_delta = tavros::math::vec2();


        /*glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/


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
        cbuf->copy_buffer(uniform_buffer, stage_buffer, 512, 0, 0);


        cbuf->begin_render_pass(msaa_pass, msaa_framebuffer);

        cbuf->bind_pipeline(msaa_pipeline);

        cbuf->bind_geometry(geometry1);

        cbuf->bind_shader_binding(shader_binding);

        cbuf->draw_indexed(model.surfaces[0].indices.size());

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

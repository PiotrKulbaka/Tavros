#include <tavros/renderer/passes/camera_pass.hpp>


namespace
{

    using namespace tavros::math;

    struct alignas(16) view_data // Mirrored 1:1 in the shader
    {
        // 3D scene
        mat4 view;
        mat4 projection;
        mat4 view_projection;
        mat4 inverse_view;
        mat4 inverse_projection;

        // 3D camera
        vec3  camera_position;
        float near_plane = 0.0f;
        float far_plane = 0.0f;
        float aspect_ratio = 0.0f;
        float fov_y = 0.0f;
        float _pad0 = 0.0f;

        // UI scene
        mat4 ui_projection;

        // Frame
        vec2 frame_size;
        vec2 frame_size_inv;

        // Time
        float  total_time = 0.0f;
        float  delta_time = 0.0f;
        uint32 frame_number = 0;
        float  _pad1 = 0.0f;

        // Debug rendering
        vec3  sun_direction;
        float sun_intensity = 0.0f;

        vec3  sun_color;
        float ambient_intensity = 0.0f;

        vec3  sky_color;
        float specular_intensity = 0.0f;

        vec3  ground_color;
        float specular_power = 0.0f;

        vec3  fill_color;
        float fill_intensity = 0.0f;

        float rim_intensity = 0.0f;
        float rim_power = 0.0f;
        float saturation = 0.0f;
        float light_wrap = 0.0f;
    };

    constexpr uint32 k_kamera_binding_index = 0;

} // namespace

namespace tavros::renderer
{

    camera_pass::camera_pass(render_system* rs)
    {
        TAV_UNUSED(rs);
    }

    camera_pass::~camera_pass() noexcept
    {
    }

    void camera_pass::upload(frame_context& ctx)
    {
        const auto& cam_data = ctx.frame_data().get_data<fd_camera>();
        const auto& cam = cam_data.cam;
        const auto& finf = ctx.frame_data().get_data<fd_frame_info>();

        auto v = view_data{
            .view = cam.view_matrix(),
            .projection = cam.projection_matrix(),
            .view_projection = cam.view_projection_matrix(),
            .inverse_projection = math::inverse(cam.view_matrix()),
            .camera_position = cam.position(),
            .near_plane = cam.perspective_params().z_near,
            .far_plane = cam.perspective_params().z_far,
            .aspect_ratio = cam.perspective_params().aspect,
            .fov_y = cam.perspective_params().fov_y,
            ._pad0 = 0.0f,
            .ui_projection = math::make_ortho(0.0f, finf.frame_size.width, finf.frame_size.height, 0.0f, 1.0f, -1.0f),
            .frame_size = finf.frame_size,
            .frame_size_inv = 1.0f / finf.frame_size,
            .total_time = finf.total_time,
            .delta_time = finf.delta_time,
            .frame_number = static_cast<uint32>(ctx.frame_number()),
            ._pad1 = 0.0f,

            .sun_direction = math::vec3(0.45f, -0.55f, 0.70f),
            .sun_intensity = 0.8f,
            .sun_color = math::vec3(1.00f, 0.96f, 0.88f),
            .ambient_intensity = 0.35f,
            .sky_color = math::vec3(0.50f, 0.68f, 1.00f),
            .specular_intensity = 0.35f,
            .ground_color = math::vec3(0.35f, 0.30f, 0.26f),
            .specular_power = 48.0f,
            .fill_color = math::vec3(0.65f, 0.78f, 1.00f),
            .fill_intensity = 0.12f,
            .rim_intensity = 0.35f,
            .rim_power = 3.0f,
            .saturation = 1.25f,
            .light_wrap = 0.3f,
        };

        auto slice = ctx.constant_buffer().slice<view_data>();
        slice.data().copy_from(&v);
        m_camera_binding = rhi::buffer_binding{
            slice.gpu_buffer(),
            static_cast<uint32>(slice.offset_bytes()),
            static_cast<uint32>(slice.size_bytes()),
            k_kamera_binding_index
        };
    }

    void camera_pass::execute(frame_context& ctx)
    {
        ctx.commands().bind_shader_buffers(m_camera_binding);
    }

} // namespace tavros::renderer

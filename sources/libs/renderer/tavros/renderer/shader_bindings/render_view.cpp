#include <tavros/renderer/shader_bindings/render_view.hpp>

namespace tavros::renderer
{

    void render_view::set_camera_data(const camera& cam) noexcept
    {
        m_data.view = cam.view_matrix();
        m_data.projection = cam.projection_matrix();
        m_data.view_projection = cam.view_projection_matrix();
        m_data.inverse_view = math::inverse(cam.view_matrix());
        m_data.inverse_projection = math::inverse(cam.projection_matrix());
        m_data.camera_position = cam.position();
        m_data.near_plane = cam.perspective_params().z_near;
        m_data.far_plane = cam.perspective_params().z_far;
        m_data.aspect_ratio = cam.perspective_params().aspect;
        m_data.fov_y = cam.perspective_params().fov_y;
    }

    void render_view::set_frame_info(math::vec2 frame_size, float total_time, float delta_time, uint64 frame_number) noexcept
    {
        m_data.ui_projection = math::make_ortho(0.0f, frame_size.width, frame_size.height, 0.0f, 1.0f, -1.0f);
        m_data.frame_size = frame_size;
        m_data.frame_size_inv = 1.0f / frame_size;
        m_data.total_time = total_time;
        m_data.delta_time = delta_time;
        m_data.frame_number = static_cast<uint32>(frame_number);
    }

    rhi::buffer_binding render_view::upload(gpu_stream_buffer& sb) const noexcept
    {
        auto slice = sb.slice<view_data>();
        slice.data().copy_from(&m_data);
        return {slice.gpu_buffer(), static_cast<uint32>(slice.offset_bytes()), static_cast<uint32>(slice.size_bytes()), k_render_view_binding_index};
    }

    const render_view::view_data& render_view::data() const noexcept
    {
        return m_data;
    }

} // namespace tavros::renderer
#include <tavros/renderer/camera/camera_controller.hpp>

namespace tavros::renderer
{

    camera_controller::camera_controller(camera& cam) noexcept
        : m_camera(cam)
    {
        const auto f = m_camera.forward();
        m_yaw = math::atan2(f.y, f.x);
        m_pitch = math::asin(math::clamp(f.z, -1.0f, 1.0f));
    }

    void camera_controller::set_world_up(const math::vec3& world_up) noexcept
    {
        m_world_up = math::normalize(world_up);
        apply();
    }

    void camera_controller::set_pivot(const math::vec3& pivot) noexcept
    {
        m_pivot = pivot;
        apply();
    }

    void camera_controller::set_orientation(const math::quat& orientation) noexcept
    {
        const math::vec3 f = orientation * math::vec3(1.0f, 0.0f, 0.0f);
        m_yaw = math::atan2(f.y, f.x);
        m_pitch = math::asin(math::clamp(f.z, -1.0f, 1.0f));
        apply();
    }

    void camera_controller::rotate(const math::vec2& delta) noexcept
    {
        m_yaw -= delta.x;
        m_pitch -= delta.y;
        m_pitch = math::clamp(m_pitch, -k_pitch_limit, k_pitch_limit);
        apply();
    }

    void camera_controller::set_zoom(float zoom_factor) noexcept
    {
        m_distance = math::clamp(zoom_factor, k_min_zoom, k_max_zoom);
        apply();
    }

    void camera_controller::zoom(float delta) noexcept
    {
        if (delta != 0.0f) {
            auto before = m_distance > 0.01f ? m_distance : 0.01f;
            m_distance -= delta * before;
            m_distance = math::clamp(m_distance, 0.0f, 1000.0f);
            apply();
        }
    }

    void camera_controller::move(const math::vec3& delta) noexcept
    {
        const auto offset =
            m_camera.forward() * delta.x + m_camera.right() * delta.y + m_camera.up() * delta.z;
        m_pivot += offset;
        apply();
    }

    void camera_controller::pan(const math::vec2& delta) noexcept
    {
        const auto offset =
            m_camera.right() * delta.x * m_distance + m_camera.up() * delta.y * m_distance;
        m_pivot += offset;
        apply();
    }

    void camera_controller::apply() noexcept
    {
        const float cy = math::cos(m_yaw), sy = math::sin(m_yaw);
        const float cp = math::cos(m_pitch), sp = math::sin(m_pitch);

        const math::vec3 forward = math::normalize(math::vec3(cp * cy, cp * sy, sp));
        const math::vec3 right = math::normalize(math::cross(m_world_up, forward));
        const math::vec3 up = math::cross(forward, right);

        m_camera.set_orientation(math::quat::from_axes(forward, right, up));
        m_camera.set_position(m_pivot - forward * m_distance);
    }

} // namespace tavros::renderer

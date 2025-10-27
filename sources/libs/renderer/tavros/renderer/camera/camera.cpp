#include <tavros/renderer/camera/camera.hpp>

using namespace tavros::math;

namespace tavros::renderer
{

    camera::camera() noexcept
        : m_position(0.0f, 0.0f, 0.0f)
        , m_forward(1.0f, 0.0f, 0.0f)
        , m_up(0.0f, 0.0f, 1.0f)
        , m_fov_y(1.0f)
        , m_aspect(1.0f)
        , m_z_near(0.1f)
        , m_z_far(1000.0f)
        , m_dirty(true)
    {
    }

    camera::camera(const vec3& position, const vec3& forward, const vec3& world_up) noexcept
        : m_position(position)
        , m_forward(normalize(forward))
        , m_up(normalize(world_up))
        , m_fov_y(1.0f)
        , m_aspect(1.0f)
        , m_z_near(0.1f)
        , m_z_far(1000.0f)
        , m_dirty(true)
    {
    }

    void camera::set_perspective(float fov_y, float aspect, float z_near, float z_far) noexcept
    {
        m_fov_y = fov_y;
        m_aspect = aspect;
        m_z_near = z_near;
        m_z_far = z_far;
        m_projection = mat4::perspective(m_fov_y, m_aspect, m_z_near, m_z_far);
        m_dirty = true;
    }

    void camera::set_position(const vec3& position) noexcept
    {
        m_position = position;
        m_dirty = true;
    }

    void camera::set_orientation(const vec3& forward, const vec3& world_up) noexcept
    {
        m_forward = normalize(forward);
        m_up = normalize(world_up);
        m_dirty = true;
    }

    void camera::move(const vec3& delta) noexcept
    {
        m_position += delta;
        m_dirty = true;
    }

    void camera::rotate(const quat& q) noexcept
    {
        m_forward = normalize(q * m_forward);
        m_up = normalize(q * m_up);
        m_dirty = true;
    }

    vec3 camera::forward() const noexcept
    {
        return m_forward;
    }

    vec3 camera::up() const noexcept
    {
        auto r = right();
        return cross(m_forward, r);
    }

    math::vec3 camera::world_up() const noexcept
    {
        return m_up;
    }

    vec3 camera::right() const noexcept
    {
        return normalize(cross(m_up, m_forward));
    }

    vec3 camera::position() const noexcept
    {
        return m_position;
    }

    mat4 camera::get_view_matrix() noexcept
    {
        if (m_dirty) {
            m_view = mat4::look_at_dir(m_position, m_forward, m_up);
            m_view_projection = m_projection * m_view;
            m_dirty = false;
        }
        return m_view;
    }

    mat4 camera::get_projection_matrix() noexcept
    {
        get_view_matrix();
        return m_projection;
    }

    mat4 camera::get_view_projection_matrix() noexcept
    {
        get_view_matrix();
        return m_view_projection;
    }

} // namespace tavros::renderer

#pragma once

#include <tavros/core/math.hpp>

namespace tavros::renderer
{

    class camera
    {
    public:
        camera() noexcept;

        camera(const math::vec3& position, const math::vec3& forward, const math::vec3& up) noexcept;

        ~camera() noexcept = default;

        void set_perspective(float fov_y, float aspect, float z_near, float z_far) noexcept;
        void set_position(const math::vec3& position) noexcept;
        void set_orientation(const math::vec3& forward, const math::vec3& up) noexcept;

        void move(const math::vec3& delta) noexcept;
        void rotate(const math::quat& q) noexcept;

        math::vec3 forward() const noexcept;
        math::vec3 up() const noexcept;
        math::vec3 right() const noexcept;
        math::vec3 position() const noexcept;

        math::mat4 get_view_matrix() noexcept;
        math::mat4 get_projection_matrix() noexcept;
        math::mat4 get_view_projection_matrix() noexcept;

    private:
        math::vec3 m_position;                // Положение камеры
        math::vec3 m_forward;                 // Направление взгляда (нормализованное)
        math::vec3 m_up;                      // Вектор "вверх" камеры (нормализованный)

        float m_fov_y;                        // Вертикальное поле зрения (в радианах)
        float m_aspect;                       // Соотношение сторон (width / height)
        float m_z_near;                       // Ближняя плоскость отсечения
        float m_z_far;                        // Дальняя плоскость отсечения

        mutable math::mat4 m_view;            // Кэш матрицы вида
        mutable math::mat4 m_projection;      // Кэш матрицы проекции
        mutable math::mat4 m_view_projection; // Кэш итоговой VP матрицы
        mutable bool       m_dirty;           // Флаг необходимости пересчёта матриц
    };

} // namespace tavros::renderer

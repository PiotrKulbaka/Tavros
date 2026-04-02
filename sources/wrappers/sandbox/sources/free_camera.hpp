#pragma once

#include <tavros/renderer/camera/camera.hpp>
#include <tavros/renderer/camera/camera_controller.hpp>
#include <tavros/input/input_manager.hpp>

namespace tavros::sandbox
{

    class free_camera
    {
    public:
        static constexpr float k_mouse_sensitivity = 0.5f;

    public:
        free_camera() noexcept
            : m_controller(m_camera)
            , m_fly(false)
        {
            m_controller.set_zoom(10.0f);
        }

        void set_pivot(const math::vec3& pos) noexcept
        {
            m_controller.set_pivot(pos);
        }

        void set_orientation(const math::quat& orientation) noexcept
        {
            m_controller.set_orientation(orientation);
        }

        void set_fly(bool fly) noexcept
        {
            m_fly = fly;
        }

        void set_perspective(float fov_y, float aspect, float z_near, float z_far) noexcept
        {
            m_camera.set_perspective({fov_y, aspect, z_near, z_far});
        }

        void update(input::input_manager& input, float delta_time) noexcept
        {
            const auto mouse_delta = input.smooth_mouse_delta();
            const auto factor = [&](input::keyboard_key key) {
                return static_cast<float>(input.key_hold_factor(key));
            };

            if (m_fly) {
                if (math::squared_length(mouse_delta) > 0.0f) {
                    m_controller.rotate(mouse_delta * k_mouse_sensitivity);
                }

                auto move = math::vec3(
                    factor(input::keyboard_key::k_W) - factor(input::keyboard_key::k_S),
                    factor(input::keyboard_key::k_D) - factor(input::keyboard_key::k_A),
                    factor(input::keyboard_key::k_space) - factor(input::keyboard_key::k_C)
                );

                const float len = math::length(move);
                if (len > 1.0f) {
                    move /= len;
                }

                const bool  shift = input.is_key_held(input::keyboard_key::k_lshift);
                const bool  ctrl = input.is_key_held(input::keyboard_key::k_lcontrol);
                const float speed = static_cast<float>(delta_time) * (shift ? (ctrl ? 100.0f : 10.0f) : 2.0f);
                m_controller.move(move * speed);
            } else {
                if (input.is_mouse_button_held(input::mouse_button::middle)) {
                    if (input.is_key_held(input::keyboard_key::k_lshift)) { // move camera
                        m_controller.pan(math::vec2(-mouse_delta.x, mouse_delta.y) * k_mouse_sensitivity * k_mouse_sensitivity);
                    } else if (math::squared_length(mouse_delta) > 0.0f) {  // rotate camera
                        m_controller.rotate(mouse_delta * k_mouse_sensitivity);
                    }
                }
            }

            m_controller.zoom(input.wheel_scroll_delta().y / 1200.0f);
        }

        const renderer::camera& camera() const noexcept
        {
            return m_camera;
        }

        const renderer::camera_controller& controller() const noexcept
        {
            return m_controller;
        }

    private:
        renderer::camera            m_camera;
        renderer::camera_controller m_controller;
        bool                        m_fly;
    };

} // namespace tavros::sandbox

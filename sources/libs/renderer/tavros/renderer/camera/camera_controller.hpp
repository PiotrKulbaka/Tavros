#pragma once

#include <tavros/renderer/camera/camera.hpp>

namespace tavros::renderer
{

    /**
     * @brief Camera controller.
     *
     * Provides high-level camera manipulation such as rotation, movement,
     * zoom and panning. Operates on a referenced camera instance.
     *
     * The controller maintains its own internal state (yaw, pitch, distance)
     * and applies it to the camera via @ref apply().
     *
     * Supports both free movement and orbit-like behavior depending on usage.
     */
    class camera_controller
    {
    public:
        /// Maximum absolute pitch angle (prevents flipping).
        static constexpr float k_pitch_limit = math::k_pi<float> * 0.49999f;

        /// Minimum allowed zoom distance.
        static constexpr float k_min_zoom = 0.0f;

        /// Maximum allowed zoom distance.
        static constexpr float k_max_zoom = 1000.0f;

    public:
        /**
         * @brief Constructs controller for a camera.
         *
         * @param cam Camera to control.
         */
        explicit camera_controller(camera& cam) noexcept;

        /**
         * @brief Sets world up direction.
         *
         * Affects orientation reconstruction (right/up vectors).
         */
        void set_world_up(const math::vec3& world_up) noexcept;

        /**
         * @brief Sets camera pivot point.
         *
         * Defines the point in world space around which the camera rotates
         * in orbit mode.
         *
         * When zoom (distance) is zero, the pivot coincides with the camera
         * position.
         *
         * Updates internal state and applies it to the camera.
         */
        void set_pivot(const math::vec3& pivot) noexcept;

        /**
         * @brief Sets camera orientation.
         *
         * Extracts yaw and pitch from the given orientation.
         */
        void set_orientation(const math::quat& orientation) noexcept;

        /**
         * @brief Rotates the camera.
         *
         * @param delta Rotation delta (yaw, pitch).
         *
         * Pitch is clamped to @ref k_pitch_limit.
         */
        void rotate(const math::vec2& delta) noexcept;

        /**
         * @brief Sets zoom (distance to pivot).
         *
         * Clamped to [@ref k_min_zoom, @ref k_max_zoom].
         */
        void set_zoom(float zoom_factor) noexcept;

        /**
         * @brief Adjusts zoom by delta.
         *
         * Positive values typically zoom out, negative zoom in.
         */
        void zoom(float delta) noexcept;

        /**
         * @brief Moves the camera in world or local space.
         *
         * Interpretation of delta depends on implementation
         * (e.g. local axes vs world axes).
         */
        void move(const math::vec3& delta) noexcept;

        /**
         * @brief Pans the camera parallel to the view plane.
         *
         * @param delta Movement along right/up axes.
         */
        void pan(const math::vec2& delta) noexcept;

    private:
        void apply() noexcept;

    private:
        camera&    m_camera;
        math::vec3 m_pivot;
        math::vec3 m_world_up = {0.0f, 0.0f, 1.0f};
        float      m_yaw = 0.0f;
        float      m_pitch = 0.0f;
        float      m_distance = 0.0f;
    };

} // namespace tavros::renderer

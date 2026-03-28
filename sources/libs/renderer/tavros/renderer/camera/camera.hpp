#pragma once

#include <tavros/core/math.hpp>

namespace tavros::renderer
{

    /**
     * @brief Camera representation.
     * A camera is defined by its position, orientation and projection.
     */
    class camera
    {
    public:
        /**
         * @brief Parameters for perspective projection.
         */
        struct perspective_params_t
        {
            float fov_y = 0.0f;  ///< Vertical field of view in radians.
            float aspect = 0.0f; ///< Aspect ratio (width / height).
            float z_near = 0.0f; ///< Near clipping plane distance.
            float z_far = 0.0f;  ///< Far clipping plane distance.
        };

        struct ortho_params_t
        {
            float left = 0.0f;   ///< Left plane.
            float right = 0.0f;  ///< Right plane.
            float bottom = 0.0f; ///< Bottom plane.
            float top = 0.0f;    ///< Top plane.
            float z_near = 0.0f; ///< Near clipping plane distance.
            float z_far = 0.0f;  ///< Far clipping plane distance.
        };

    public:
        /**
         * @brief Default constructor.
         */
        camera() noexcept = default;

        /**
         * @brief Constructs a camera with given transform.
         *
         * @param position World-space position.
         * @param orientation World-space orientation.
         */
        camera(const math::vec3& position, const math::quat& orientation) noexcept
        {
            set_position(position);
            set_orientation(orientation);
        }

        /**
         * @brief Sets camera position.
         */
        void set_position(const math::vec3& position) noexcept
        {
            m_position = position;
        }

        /**
         * @brief Sets camera orientation. The quaternion is normalized internally.
         */
        void set_orientation(const math::quat& orientation) noexcept
        {
            m_orientation = normalize(orientation);
        }

        /**
         * @brief Returns camera position.
         */
        [[nodiscard]] math::vec3 position() const noexcept
        {
            return m_position;
        }

        /**
         * @brief Returns camera orientation.
         */
        [[nodiscard]] math::quat orientation() const noexcept
        {
            return m_orientation;
        }

        /**
         * @brief Returns forward direction vector.
         *
         * Assumes +X is forward in local space.
         */
        [[nodiscard]] math::vec3 forward() const noexcept
        {
            return m_orientation * math::vec3(1.0f, 0.0f, 0.0f);
        }

        /**
         * @brief Returns right direction vector.
         *
         * Assumes -Y is right in local space.
         */
        [[nodiscard]] math::vec3 right() const noexcept
        {
            return m_orientation * math::vec3(0.0f, -1.0f, 0.0f);
        }

        /**
         * @brief Returns up direction vector.
         *
         * Assumes +Z is up in local space.
         */
        [[nodiscard]] math::vec3 up() const noexcept
        {
            return m_orientation * math::vec3(0.0f, 0.0f, 1.0f);
        }

        /**
         * @brief Sets perspective projection.
         *
         * Updates projection matrix.
         */
        void set_perspective(const perspective_params_t& cam_params) noexcept
        {
            m_perspective_params = cam_params;
            m_projection = math::mat4::perspective(
                cam_params.fov_y,
                cam_params.aspect,
                cam_params.z_near,
                cam_params.z_far
            );
            m_is_perspective = true;
        }

        /**
         * @brief Sets orthographic projection.
         *
         * Updates projection matrix.
         */
        void set_ortho(const ortho_params_t& cam_params) noexcept
        {
            m_ortho_params = cam_params;
            m_projection = math::mat4::ortho(
                cam_params.left,
                cam_params.right,
                cam_params.bottom,
                cam_params.top,
                cam_params.z_near,
                cam_params.z_far
            );
            m_is_perspective = false;
        }

        /**
         * @brief Returns perspective parameters.
         */
        [[nodiscard]] const perspective_params_t& perspective_params() const noexcept
        {
            return m_perspective_params;
        }

        /**
         * @brief Returns orthographic parameters.
         */
        [[nodiscard]] const ortho_params_t& ortho_params() const noexcept
        {
            return m_ortho_params;
        }

        /**
         * @brief Returns true if perspective projection is active.
         */
        [[nodiscard]] bool is_perspective() const noexcept
        {
            return m_is_perspective;
        }

        /**
         * @brief Computes view matrix.
         *
         * Uses position, forward and up vectors.
         */
        [[nodiscard]] math::mat4 view_matrix() const noexcept
        {
            return math::mat4::look_at_dir(m_position, forward(), up());
        }

        /**
         * @brief Returns projection matrix.
         */
        [[nodiscard]] math::mat4 projection_matrix() const noexcept
        {
            return m_projection;
        }

        /**
         * @brief Returns combined view-projection matrix.
         */
        [[nodiscard]] math::mat4 view_projection_matrix() const noexcept
        {
            return projection_matrix() * view_matrix();
        }

    private:
        math::vec3 m_position;
        math::quat m_orientation;

        math::mat4 m_projection = math::mat4::identity();

        perspective_params_t m_perspective_params;
        ortho_params_t       m_ortho_params;
        bool                 m_is_perspective = true;
    };

} // namespace tavros::renderer

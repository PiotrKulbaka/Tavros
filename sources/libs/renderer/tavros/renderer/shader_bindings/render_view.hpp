#pragma once

#include <tavros/renderer/camera/camera.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>
#include <tavros/renderer/shader_bindings/shader_binding_indices.hpp>

namespace tavros::renderer
{

    /**
     * @brief Stores global view and frame data shared by all shaders.
     *
     * Maintains camera matrices, frame information, and timing data that
     * are uploaded to the GPU and mapped to the `Scene` shader structure.
     */
    class render_view
    {
    public:
        /**
         * @brief Data uploaded to the GPU and mapped to the `Scene` shader structure.
         *
         * Contains camera matrices, camera parameters, UI projection,
         * frame information, and timing data.
         */
        struct alignas(16) view_data // Mirrored 1:1 in the shader
        {
            // 3D scene
            math::mat4 view;
            math::mat4 projection;
            math::mat4 view_projection;
            math::mat4 inverse_view;
            math::mat4 inverse_projection;

            // 3D camera
            math::vec3 camera_position;
            float      near_plane = 0.0f;
            float      far_plane = 0.0f;
            float      aspect_ratio = 0.0f;
            float      fov_y = 0.0f;
            float      _pad0 = 0.0f;

            // UI scene
            math::mat4 ui_projection;

            // Frame
            math::vec2 frame_size;
            math::vec2 frame_size_inv;

            // Time
            float  total_time = 0.0f;
            float  delta_time = 0.0f;
            uint32 frame_number = 0;
            float  _pad1 = 0.0f;
        };

    public:
        /** @brief Constructs an empty render view. */
        render_view() noexcept = default;

        /**
         * @brief Updates the view using the specified camera.
         *
         * Computes all camera-dependent matrices and parameters.
         */
        void set_camera_data(const camera& cam) noexcept;

        /**
         * @brief Updates frame-dependent information.
         *
         * Sets the frame size, timing information, and frame number.
         */
        void set_frame_info(
            math::vec2 frame_size,
            float      total_time,
            float      delta_time,
            uint64     frame_number
        ) noexcept;

        /**
         * @brief Uploads the view data to the specified GPU stream buffer.
         *
         * Returns a buffer binding that can be bound to shaders.
         */
        rhi::buffer_binding upload(gpu_stream_buffer& sb) const noexcept;

        /** @brief Returns the current view data. */
        const view_data& data() const noexcept;

    private:
        view_data m_data;
    };

} // namespace tavros::renderer

#pragma once

#include <tavros/core/math/mat4.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec2.hpp>

namespace app
{

    struct alignas(16) scene_data
    {
        // Matrices
        tavros::math::mat4 view;
        tavros::math::mat4 projection;
        tavros::math::mat4 view_projection;
        tavros::math::mat4 inverse_view;
        tavros::math::mat4 inverse_projection;
        tavros::math::mat4 ortho_projection;

        // Camera
        tavros::math::vec3 camera_position;
        float              near_plane;

        float far_plane;
        float aspect_ratio;
        float fov_y;
        float _pad0 = 0.0f;

        // Frame
        tavros::math::vec2 frame_size;
        tavros::math::vec2 frame_size_inv;

        float    time = 0.0f;
        float    delta_time = 0.0f;
        uint32_t frame_index = 0;
        float    _pad1 = 0.0f;
    };

    static_assert(sizeof(scene_data) % 16 == 0, "scene_data must be 16-byte aligned");

} // namespace app

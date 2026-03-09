#pragma once

#include <tavros/core/archetype.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>

namespace app
{

    // -------------------------------------------------------------------------
    // Components
    // -------------------------------------------------------------------------

    struct position_c
    {
        tavros::math::vec3 value;
    };

    struct velocity_c
    {
        tavros::math::vec3 value;
    };

    struct color_c
    {
        tavros::math::color start;
        tavros::math::color end;

        [[nodiscard]] tavros::math::color color(float t) const noexcept
        {
            return tavros::math::lerp(start, end, t);
        }
    };

    struct size_c
    {
        float start = 0.0f;
        float end = 0.0f;
    };

    /**
     * @brief Normalized lifetime in [0..1].
     *
     * 0 = just spawned, 1 = expired.
     * Updated each frame as elapsed / total.
     * Particle is removed when value >= 1.0.
     */
    struct lifetime_c
    {
        float elapsed = 0.0f;
        float total = 0.0f;

        [[nodiscard]] float normalized() const noexcept
        {
            return elapsed / total;
        }

        [[nodiscard]] bool is_dead() const noexcept
        {
            return elapsed >= total;
        }
    };

    struct rotation_c
    {
        float value = 0.0f;
    };

    struct angular_velocity_c
    {
        float value = 0.0f;
    };

    // -------------------------------------------------------------------------
    // Archetype
    // -------------------------------------------------------------------------

    /**
     * @brief Archetype storing all per-particle components (SoA layout).
     *
     * Each frame:
     *   1. Update lifetime, position, rotation via a view over the archetype.
     *   2. Upload instance data to gpu_stream_buffer.
     *   3. Draw with draw(4, 0, size(), 0) using triangle_strip.
     */
    using particle_archetype = tavros::core::basic_archetype<
        position_c,
        velocity_c,
        color_c,
        size_c,
        lifetime_c,
        rotation_c,
        angular_velocity_c>;

    // -------------------------------------------------------------------------
    // GPU instance struct (matches layout in particle.vert)
    // -------------------------------------------------------------------------

    /**
     * @brief Per-instance data uploaded to the GPU each frame.
     *
     * Attribute layout:
     *   location 0 - position  (vec3)
     *   location 1 - size      (float, CPU-interpolated)
     *   location 2 - color     (vec4,  CPU-interpolated)
     *   location 3 - rotation  (float)
     */
    struct particle_instance
    {
        float position[3]; // location 0
        float size;        // location 1
        float color[4];    // location 2
        float rotation;    // location 3
    };

    static_assert(sizeof(particle_instance) == 9 * sizeof(float));

} // namespace app

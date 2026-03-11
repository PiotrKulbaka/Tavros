#pragma once

#include <tavros/core/archetype.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>
#include <tavros/core/math/rgba8.hpp>

namespace tavros::particles
{

    struct position_c
    {
        math::vec3 value;
    };

    struct velocity_c
    {
        math::vec3 value;
    };

    struct physics_c
    {
        float mass = 0.0f;
        float drag_coeff = 0.0f;
        float cross_area = 0.0f;
    };

    struct color_c
    {
        static constexpr int k_max_colors = 4;

        float       stops[k_max_colors] = {0.0f};
        math::rgba8 colors[k_max_colors] = {};

        [[nodiscard]] math::rgba8 sample(float t) const noexcept
        {
            if (t <= stops[0]) {
                return colors[0];
            }
            if (t >= stops[k_max_colors - 1]) {
                return colors[k_max_colors - 1];
            }

            for (int i = 0; i < k_max_colors - 1; ++i) {
                if (t <= stops[i + 1]) {
                    const float range = stops[i + 1] - stops[i];
                    const float local = (range > 1e-6f) ? (t - stops[i]) / range : 0.0f;
                    return math::rgba8::lerp(colors[i], colors[i + 1], local);
                }
            }

            return colors[k_max_colors - 1];
        }
    };

    struct size_c
    {
        float start = 0.0f;
        float end = 0.0f;

        [[nodiscard]] float sample(float t) const noexcept
        {
            return start + (end - start) * t;
        }
    };

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

    /**
     * @brief Accumulated force vector (N) applied this frame.
     *
     * Cleared to zero at the start of each physics tick.
     * Each force system (gravity, wind, explosion, ...) adds its contribution.
     * The integrator then computes: a = force / mass, v += a * dt.
     */
    struct force_c
    {
        tavros::math::vec3 value = {0.0f, 0.0f, 0.0f};
    };

    // -------------------------------------------------------------------------
    // Archetype
    // -------------------------------------------------------------------------
    using particle_archetype = core::basic_archetype<
        position_c,
        velocity_c,
        physics_c,
        force_c,
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
        float  position[3]; // location 0
        float  size;        // location 1
        uint32 color;       // location 2
        float  rotation;    // location 3
    };

    static_assert(sizeof(particle_instance) == 5 * sizeof(float) + sizeof(uint32));

} // namespace tavros::particles

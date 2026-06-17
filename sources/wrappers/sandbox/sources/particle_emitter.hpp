#pragma once

#include "particle_components.hpp"
#include "particle_shapes.hpp"
#include "particle_physics.hpp"

#include <tavros/core/math/rgba8.hpp>
#include <array>

namespace tavros::particles
{

    struct color_range
    {
        struct stop_range
        {
            float       t = 0.0f;
            math::rgba8 color_min;
            math::rgba8 color_max;
        };

        static constexpr int k_max = color0_c::k_max_colors;
        stop_range           stops[k_max];

        template<typename Rng>
        [[nodiscard]] color0_c sample(Rng&& rng) const noexcept
        {
            color0_c result;
            for (int i = 0; i < k_max; ++i) {
                result.stops[i] = stops[i].t;
                result.colors[i] = math::rgba8::lerp(stops[i].color_min, stops[i].color_max, rng(0.0f, 1.0f));
            }
            return result;
        }
    };

    struct velocity_range
    {
        float      speed_min = 0.0f;
        float      speed_max = 1.0f;
        math::vec3 direction = {0.0f, 0.0f, 1.0f};
        float      spread_angle = 3.14159f;

        template<typename Rng>
        [[nodiscard]] math::vec3 sample(Rng&& rng) const noexcept
        {
            const float speed = rng(speed_min, speed_max);

            if (spread_angle >= 3.14159f) {
                const float az = rng(-3.14159f, 3.14159f);
                const float el = std::asin(rng(-1.0f, 1.0f));
                return {
                    speed * std::cos(el) * std::cos(az),
                    speed * std::cos(el) * std::sin(az),
                    speed * std::sin(el),
                };
            }

            const float angle = rng(0.0f, spread_angle);
            const float az = rng(0.0f, 6.2831f);
            const float sin_a = std::sin(angle);

            const math::vec3 d = math::normalize(direction);
            const math::vec3 perp = (std::abs(d.z) < 0.99f)
                                      ? math::normalize(math::cross(d, {0.0f, 0.0f, 1.0f}))
                                      : math::normalize(math::cross(d, {1.0f, 0.0f, 0.0f}));
            const math::vec3 bitangent = math::cross(d, perp);

            return (d * std::cos(angle) + (perp * std::cos(az) + bitangent * std::sin(az)) * sin_a) * speed;
        }
    };

    struct particle_params
    {
        physics_c      physics = k_ember_preset;
        color_range    colors = {};
        velocity_range velocity = {};
        float          lifetime_min = 1.0f;
        float          lifetime_max = 3.0f;
        float          size_start_min = 0.05f;
        float          size_start_max = 0.15f;
        float          size_end_min = 0.0f;
        float          size_end_max = 0.0f;
        float          rot_min = 0.0f;
        float          rot_max = 6.2831f;
        float          avel_min = -2.0f;
        float          avel_max = 2.0f;
    };

    enum class emitter_mode : uint8_t
    {
        stream,
        burst,
    };

    // -------------------------------------------------------------------------
    // Emitter
    // -------------------------------------------------------------------------

    struct emitter_c
    {
        spawn_shape        shape = spawn_shape::point;
        spawn_shape_params shape_params = {};
        particle_params    params = {};
        emitter_mode       mode = emitter_mode::stream;

        float rate_min = 10.0f; // particles/sec (stream)
        float rate_max = 20.0f; // particles/sec

        float lifetime = 5.0f;
        bool  immortal = false;

        // --- state ---
        float elapsed = 0.0f;
        float accumulator = 0.0f;
        bool  active = true;
        bool  burst_fired = false; // for mode::burst

        [[nodiscard]] bool is_dead() const noexcept
        {
            return !immortal && elapsed >= lifetime;
        }
    };

    using emitter_archetype = tavros::core::basic_archetype<emitter_c>;

    // -------------------------------------------------------------------------
    // Emitter systems
    // -------------------------------------------------------------------------

    template<typename Rng>
    inline void spawn_particle(particle_archetype& particles, const emitter_c& em, math::vec3 pos, Rng&& rng) noexcept
    {
        const auto& p = em.params;
        particles.typed_emplace_back(
            position_c{pos},
            velocity_c{p.velocity.sample(rng)},
            p.physics,
            force_c{},
            p.colors.sample(rng),
            size_c{rng(p.size_start_min, p.size_start_max), rng(p.size_end_min, p.size_end_max)},
            lifetime_c{0.0f, rng(p.lifetime_min, p.lifetime_max)},
            rotation_c{rng(p.rot_min, p.rot_max)},
            angular_velocity_c{rng(p.avel_min, p.avel_max)}
        );
    }

    template<tavros::core::archetype_with<emitter_c> T, typename Rng>
    inline void update_emitters(T& emitters, particle_archetype& particles, float dt, Rng&& rng) noexcept
    {
        emitters.view<emitter_c>().each([&](auto& em) {
            if (!em.active || em.is_dead()) {
                return;
            }

            em.elapsed += dt;

            if (em.mode == emitter_mode::burst) {
                if (!em.burst_fired) {
                    const int count = static_cast<int>(rng(em.rate_min, em.rate_max));
                    for (int j = 0; j < count; ++j) {
                        spawn_particle(particles, em, sample_shape(em.shape, em.shape_params, rng), rng);
                    }
                    em.burst_fired = true;
                }
            } else {
                // stream
                const float rate = rng(em.rate_min, em.rate_max);
                em.accumulator += rate * dt;

                const int count = static_cast<int>(em.accumulator);
                em.accumulator -= static_cast<float>(count);

                for (int j = 0; j < count; ++j) {
                    spawn_particle(particles, em, sample_shape(em.shape, em.shape_params, rng), rng);
                }
            }
        });

        // Clean dead emitters
        auto em_v = emitters.get<emitter_c>();
        for (size_t i = emitters.size(); i-- > 0;) {
            if (em_v[i].is_dead()) {
                emitters.swap_and_pop(i);
            }
        }
    }

} // namespace tavros::particles

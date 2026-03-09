#pragma once

#include "particle_components.hpp"

#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/math/vec3.hpp>

#include <cmath>

namespace app
{

    // -------------------------------------------------------------------------
    // Simulation
    // -------------------------------------------------------------------------

    /**
     * @brief Advances particle simulation by @p dt seconds.
     *        Dead particles are removed via swap-and-pop.
     */
    inline void update_particles(particle_archetype& archetype, float dt) noexcept
    {
        archetype
            .view<position_c, velocity_c, rotation_c, angular_velocity_c, lifetime_c>()
            .each([dt](auto& pos, auto& vel, auto& rot, auto& avel, auto& lt) {
                pos.value.x += vel.value.x * dt;
                pos.value.y += vel.value.y * dt;
                pos.value.z += vel.value.z * dt;
                rot.value += avel.value * dt;
                lt.elapsed += dt;
            });

        auto lt_v = archetype.get<lifetime_c>();
        for (size_t i = archetype.size(); i-- > 0;) {
            if (lt_v[i].is_dead()) {
                archetype.swap_and_pop(i);
            }
        }
    }

    /**
     * @brief Fills @p dst with GPU instance data from @p archetype.
     * @return Number of instances written.
     */
    inline size_t fill_particle_instances(
        particle_archetype&                          archetype,
        tavros::core::buffer_span<particle_instance> dst
    ) noexcept
    {
        size_t count = 0;
        auto*  out = dst.begin();

        archetype
            .view<const position_c, const color_c, const size_c, const lifetime_c, const rotation_c>()
            .each([&](const auto& pos, const color_c& col, const auto& sz, const auto& lt, const auto& rot) {
                const float t = lt.normalized();
                const auto  cl = col.color(t);

                out->position[0] = pos.value.x;
                out->position[1] = pos.value.y;
                out->position[2] = pos.value.z;

                out->color[0] = cl.r;
                out->color[1] = cl.g;
                out->color[2] = cl.b;
                out->color[3] = cl.a;

                out->size = sz.start + (sz.end - sz.start) * t;
                out->rotation = rot.value;

                ++out;
                ++count;
            });

        return count;
    }

    // -------------------------------------------------------------------------
    // Spawner
    // -------------------------------------------------------------------------

    /**
     * @brief Stateless factory for all particle effects.
     *
     * Every method takes a @p rng callable of signature
     *   float rng(float lo, float hi)
     * and appends new particles directly into @p particles.
     */
    class particle_spawner
    {
    public:
        /// Spherical burst from a point (one-shot).
        template<typename Rng>
        static void explosion(
            particle_archetype& particles,
            tavros::math::vec3  origin,
            int                 count,
            Rng&&               rng
        ) noexcept
        {
            for (int i = 0; i < count; ++i) {
                const float az = rng(-3.14159f, 3.14159f);
                const float el = rng(-1.5708f, 1.5708f);
                const float speed = rng(2.0f, 12.0f);

                const float cx = std::cos(el) * std::cos(az);
                const float cy = std::cos(el) * std::sin(az);
                const float cz = std::sin(el);

                particles.typed_emplace_back(
                    position_c{origin},
                    velocity_c{{cx * speed, cy * speed + rng(0.0f, 4.0f), cz * speed}},
                    color_c{{rng(0.9f, 1.0f), rng(0.4f, 0.7f), 0.0f, 1.0f}, {0.6f, 0.0f, 0.0f, 0.0f}},
                    size_c{rng(0.05f, 0.25f), 0.0f},
                    lifetime_c{0.0f, rng(1.0f, 3.0f)},
                    rotation_c{rng(0.0f, 6.2831f)},
                    angular_velocity_c{rng(-5.0f, 5.0f)}
                );
            }
        }

        /// Continuous upward fountain (call every frame).
        template<typename Rng>
        static void fountain(
            particle_archetype& particles,
            tavros::math::vec3  origin,
            int                 count,
            Rng&&               rng
        ) noexcept
        {
            for (int i = 0; i < count; ++i) {
                const float angle = rng(0.0f, 6.2831f);
                const float spread = rng(0.0f, 0.8f);
                const float speed = rng(5.0f, 14.0f);

                particles.typed_emplace_back(
                    position_c{origin},
                    velocity_c{{std::cos(angle) * spread * speed * 0.3f, std::sin(angle) * spread * speed * 0.3f, speed}},
                    color_c{{0.2f, 0.5f, rng(0.8f, 1.0f), 1.0f}, {0.0f, 0.0f, 0.3f, 0.0f}},
                    size_c{rng(0.03f, 0.12f), 0.01f},
                    lifetime_c{0.0f, rng(1.2f, 2.5f)},
                    rotation_c{rng(0.0f, 6.2831f)},
                    angular_velocity_c{rng(-2.0f, 2.0f)}
                );
            }
        }

        /// Magical spiral vortex around the Y axis (call every frame).
        template<typename Rng>
        static void vortex(
            particle_archetype& particles,
            tavros::math::vec3  origin,
            float               time,
            int                 count,
            Rng&&               rng
        ) noexcept
        {
            const float step = 6.2831f / static_cast<float>(count);

            for (int i = 0; i < count; ++i) {
                const float angle = time * 3.0f + static_cast<float>(i) * step;
                const float radius = rng(0.5f, 2.0f);
                const float speed = rng(2.0f, 5.0f);

                const float px = origin.x + std::cos(angle) * radius;
                const float pz = origin.z + std::sin(angle) * radius;

                particles.typed_emplace_back(
                    position_c{{px, origin.y + rng(-0.2f, 0.2f), pz}},
                    velocity_c{{-std::sin(angle) * speed, rng(1.0f, 4.0f), std::cos(angle) * speed}},
                    color_c{{rng(0.5f, 0.8f), 0.0f, rng(0.8f, 1.0f), 1.0f}, {0.0f, 0.0f, 0.2f, 0.0f}},
                    size_c{rng(0.04f, 0.15f), 0.0f},
                    lifetime_c{0.0f, rng(0.8f, 2.0f)},
                    rotation_c{angle},
                    angular_velocity_c{rng(-4.0f, 4.0f)}
                );
            }
        }

        /// Lightning strike along a zigzag path (one-shot).
        template<typename Rng>
        static void lightning(
            particle_archetype& particles,
            tavros::math::vec3  origin,
            float               height,
            Rng&&               rng
        ) noexcept
        {
            constexpr int   k_segments = 12;
            constexpr int   k_per_seg = 40;
            constexpr float k_jitter = 1.6f;

            tavros::math::vec3 prev = {origin.x, origin.y, origin.z + height};

            for (int seg = 0; seg < k_segments; ++seg) {
                const float t = static_cast<float>(seg + 1) / static_cast<float>(k_segments);

                const tavros::math::vec3 next = {
                    origin.x + rng(-k_jitter, k_jitter),
                    origin.y + rng(-k_jitter, k_jitter),
                    origin.z + height * (1.0f - t)
                };

                for (int i = 0; i < k_per_seg; ++i) {
                    const float              u = rng(0.0f, 1.0f);
                    const tavros::math::vec3 p = {
                        prev.x + (next.x - prev.x) * u + rng(-0.05f, 0.05f),
                        prev.y + (next.y - prev.y) * u + rng(-0.05f, 0.05f),
                        prev.z + (next.z - prev.z) * u + rng(-0.05f, 0.05f)
                    };

                    particles.typed_emplace_back(
                        position_c{p},
                        velocity_c{{rng(-0.3f, 0.3f), rng(-0.5f, 0.5f), rng(-0.3f, 0.3f)}},
                        color_c{{0.8f, 0.9f, 1.0f, 1.0f}, {0.1f, 0.2f, 1.0f, 0.0f}},
                        size_c{rng(0.02f, 0.08f), 0.0f},
                        lifetime_c{0.0f, rng(0.08f, 0.25f)},
                        rotation_c{rng(0.0f, 6.2831f)},
                        angular_velocity_c{0.0f}
                    );
                }

                prev = next;
            }
        }

        /// Swirling portal ring (call every frame).
        template<typename Rng>
        static void portal(
            particle_archetype& particles,
            tavros::math::vec3  center,
            float               radius,
            float               time,
            int                 count,
            Rng&&               rng
        ) noexcept
        {
            const float step = 6.2831f / static_cast<float>(count);

            for (int i = 0; i < count; ++i) {
                const float angle = time * 2.5f + static_cast<float>(i) * step;
                const float r = radius + rng(-0.15f, 0.15f);

                const float px = center.x + std::cos(angle) * r;
                const float pz = center.z + std::sin(angle) * r;

                constexpr float k_tangent_speed = 2.5f;
                const float     inward_speed = rng(0.3f, 1.2f);

                particles.typed_emplace_back(
                    position_c{{px, center.y + rng(-0.1f, 0.1f), pz}},
                    velocity_c{{-std::sin(angle) * k_tangent_speed + (center.x - px) * inward_speed, rng(-0.2f, 0.2f), std::cos(angle) * k_tangent_speed + (center.z - pz) * inward_speed}},
                    color_c{{rng(0.4f, 0.7f), 0.0f, rng(0.8f, 1.0f), 1.0f}, {1.0f, 0.0f, 1.0f, 0.0f}},
                    size_c{rng(0.03f, 0.1f), 0.005f},
                    lifetime_c{0.0f, rng(0.4f, 1.2f)},
                    rotation_c{angle},
                    angular_velocity_c{rng(-6.0f, 6.0f)}
                );
            }
        }

        /// Glowing trail behind a moving object (call every frame).
        template<typename Rng>
        static void meteor_trail(
            particle_archetype& particles,
            tavros::math::vec3  head,
            tavros::math::vec3  velocity,
            int                 count,
            Rng&&               rng
        ) noexcept
        {
            const float len = std::sqrt(
                velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z
            );

            if (len < 0.0001f) {
                return;
            }

            const float nx = -velocity.x / len;
            const float ny = -velocity.y / len;
            const float nz = -velocity.z / len;

            for (int i = 0; i < count; ++i) {
                const float offset = rng(0.0f, 0.8f);

                particles.typed_emplace_back(
                    position_c{{head.x + nx * offset + rng(-0.05f, 0.05f), head.y + ny * offset + rng(-0.05f, 0.05f), head.z + nz * offset + rng(-0.05f, 0.05f)}},
                    velocity_c{{nx * rng(0.5f, 2.0f) + rng(-0.3f, 0.3f), ny * rng(0.5f, 2.0f) + rng(-0.3f, 0.3f), nz * rng(0.5f, 2.0f) + rng(-0.3f, 0.3f)}},
                    color_c{{1.0f, rng(0.7f, 1.0f), rng(0.0f, 0.3f), 1.0f}, {0.8f, 0.1f, 0.0f, 0.0f}},
                    size_c{rng(0.04f, 0.18f) * (1.0f - offset), 0.0f},
                    lifetime_c{0.0f, rng(0.2f, 0.6f)},
                    rotation_c{rng(0.0f, 6.2831f)},
                    angular_velocity_c{rng(-3.0f, 3.0f)}
                );
            }
        }

        /// Slow-falling snowflakes over a rectangular area (call every frame).
        template<typename Rng>
        static void snowfall(
            particle_archetype& particles,
            tavros::math::vec3  area_center,
            float               area_width,
            float               area_depth,
            float               spawn_height,
            int                 count,
            Rng&&               rng
        ) noexcept
        {
            for (int i = 0; i < count; ++i) {
                particles.typed_emplace_back(
                    position_c{{area_center.x + rng(-area_width * 0.5f, area_width * 0.5f), area_center.y + rng(-area_depth * 0.5f, area_depth * 0.5f), area_center.z + spawn_height}},
                    velocity_c{{rng(-0.3f, 0.3f), rng(-0.3f, 0.3f), rng(-1.5f, -0.5f)}},
                    color_c{{0.85f, 0.92f, 1.0f, 1.0f}, {0.7f, 0.85f, 1.0f, 0.0f}},
                    size_c{rng(0.02f, 0.07f), rng(0.01f, 0.04f)},
                    lifetime_c{0.0f, rng(4.0f, 8.0f)},
                    rotation_c{rng(0.0f, 6.2831f)},
                    angular_velocity_c{rng(-0.5f, 0.5f)}
                );
            }
        }

        /// Rising smoke plume that expands over time (call every frame).
        template<typename Rng>
        static void smoke(
            particle_archetype& particles,
            tavros::math::vec3  origin,
            int                 count,
            Rng&&               rng
        ) noexcept
        {
            for (int i = 0; i < count; ++i) {
                const float angle = rng(0.0f, 6.2831f);
                const float r = rng(0.0f, 0.2f);

                particles.typed_emplace_back(
                    position_c{{origin.x + std::cos(angle) * r, origin.y + std::sin(angle) * r, origin.z}},
                    velocity_c{{rng(-0.3f, 0.3f), rng(-0.3f, 0.3f), rng(0.5f, 1.5f)}},
                    color_c{{0.15f, 0.15f, 0.15f, 0.6f}, {0.4f, 0.4f, 0.4f, 0.0f}},
                    size_c{rng(0.1f, 0.3f), rng(1.5f, 2.9f)}, // grows over lifetime
                    lifetime_c{0.0f, rng(3.0f, 6.0f)},
                    rotation_c{rng(0.0f, 6.2831f)},
                    angular_velocity_c{rng(-0.3f, 0.3f)}
                );
            }
        }

        /// Rainbow star burst with multiple rays (one-shot).
        template<typename Rng>
        static void star_burst(
            particle_archetype& particles,
            tavros::math::vec3  origin,
            int                 ray_count,
            Rng&&               rng
        ) noexcept
        {
            constexpr int k_per_ray = 20;
            const float   ray_step = 6.2831f / static_cast<float>(ray_count);

            for (int ray = 0; ray < ray_count; ++ray) {
                const float az = static_cast<float>(ray) * ray_step;
                const float el = rng(-0.4f, 0.4f);
                const float speed = rng(4.0f, 10.0f);

                const float dx = std::cos(el) * std::cos(az);
                const float dy = std::sin(el);
                const float dz = std::cos(el) * std::sin(az);

                // HSV-to-RGB (hue only, S=V=1)
                const float hue = static_cast<float>(ray) / static_cast<float>(ray_count);
                const float r = std::clamp(std::abs(hue * 6.0f - 3.0f) - 1.0f, 0.0f, 1.0f);
                const float g = std::clamp(2.0f - std::abs(hue * 6.0f - 2.0f), 0.0f, 1.0f);
                const float b = std::clamp(2.0f - std::abs(hue * 6.0f - 4.0f), 0.0f, 1.0f);

                for (int j = 0; j < k_per_ray; ++j) {
                    const float delay = static_cast<float>(j) / static_cast<float>(k_per_ray);

                    particles.typed_emplace_back(
                        position_c{origin},
                        velocity_c{{dx * speed, dy * speed, dz * speed}},
                        color_c{{r, g, b, 1.0f}, {r * 0.3f, g * 0.3f, b * 0.3f, 0.0f}},
                        size_c{rng(0.06f, 0.15f) * (1.0f - delay * 0.5f), 0.0f},
                        lifetime_c{delay * 0.3f, rng(0.6f, 1.5f)},
                        rotation_c{rng(0.0f, 6.2831f)},
                        angular_velocity_c{rng(-8.0f, 8.0f)}
                    );
                }
            }
        }
    };

} // namespace app

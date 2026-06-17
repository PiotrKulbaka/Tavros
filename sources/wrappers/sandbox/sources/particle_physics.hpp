#pragma once

#include "particle_components.hpp"
#include <cmath>

namespace tavros::particles
{
    inline constexpr float k_air_density = 1.225f; // kg/m^3

    inline constexpr physics_c k_ember_preset = {.mass = 0.008f, .drag_coeff = 0.47f, .cross_area = 0.0004f};
    inline constexpr physics_c k_snowflake_preset = {.mass = 0.0001f, .drag_coeff = 1.17f, .cross_area = 0.002f};
    inline constexpr physics_c k_smoke_preset = {.mass = -0.0002f, .drag_coeff = 1.5f, .cross_area = 0.02f};
    inline constexpr physics_c k_bubble_preset = {.mass = -0.001f, .drag_coeff = 0.47f, .cross_area = 0.0008f};
    inline constexpr physics_c k_spark_preset = {.mass = 0.001f, .drag_coeff = 0.47f, .cross_area = 0.0001f};
    inline constexpr physics_c k_droplet_preset = {.mass = 0.005f, .drag_coeff = 0.47f, .cross_area = 0.0008f};
    inline constexpr physics_c k_stardust_preset = {.mass = 0.002f, .drag_coeff = 0.8f, .cross_area = 0.001f};
    inline constexpr physics_c k_plasma_preset = {.mass = 0.0003f, .drag_coeff = 1.2f, .cross_area = 0.003f};

    // -------------------------------------------------------------------------
    // Systems
    // -------------------------------------------------------------------------
    template<core::archetype_with<force_c> T>
    inline void clear_forces(T& arch, size_t begin, size_t count) noexcept
    {
        arch.view<force_c>().each_n(begin, count, [](auto& f) {
            f.value.set(0.0f, 0.0f, 0.0f);
        });
    }

    // F = m*g
    template<core::archetype_with<force_c, physics_c> T>
    inline void apply_gravity(T& arch, size_t begin, size_t count) noexcept
    {
        constexpr float k_gravity = -9.81f;
        arch.view<force_c, physics_c>().each_n(begin, count, [](auto& f, auto& p) {
            f.value.z += p.mass * k_gravity;
        });
    }

    // F_drag = -1/2pCdA|v|^2·v
    template<core::archetype_with<force_c, velocity_c, physics_c> T>
    inline void apply_drag(T& arch, size_t begin, size_t count) noexcept
    {
        arch.view<force_c, velocity_c, physics_c>().each_n(begin, count, [](auto& f, auto& v, auto& p) {
            const float speed_sq = math::squared_length(v.value);
            if (speed_sq < 1e-8f) {
                return;
            }

            const float speed = std::sqrt(speed_sq);
            const float drag_mag = 0.5f * k_air_density * p.drag_coeff * p.cross_area * speed_sq;
            const float inv_speed = 1.0f / speed;

            f.value -= v.value * (drag_mag * inv_speed);
        });
    }

    //
    template<core::archetype_with<position_c, velocity_c, force_c, physics_c, lifetime_c> T>
    inline void integrate(T& arch, float dt, size_t begin, size_t count) noexcept
    {
        auto view = arch.view<position_c, velocity_c, force_c, physics_c, lifetime_c>();
        view.each_n(begin, count, [dt](auto& pos, auto& vel, auto& f, auto& ph, auto& lt) {
            const float inv_mass = 1.0f / std::abs(ph.mass);
            vel.value += f.value * (inv_mass * dt);
            pos.value += vel.value * dt;
            lt.elapsed += dt;
        });
    }

    template<core::archetype_with<position_c, velocity_c, force_c, physics_c, lifetime_c> T>
    inline void integrate_lt(T& arch, size_t begin, size_t count) noexcept
    {
        auto view = arch.view<lifetime_c, computed_c, color0_c, size_c>();
        view.each_n(begin, count, [](auto& lt, auto& cc, auto& c, auto& sz) {
            float t = lt.normalized();
            cc.color = c.sample(t).color;
            cc.size = sz.sample(t);
        });
    }

    template<tavros::core::archetype_with<rotation_c, angular_velocity_c> T>
    inline void integrate_rotation(T& arch, float dt, size_t begin, size_t count) noexcept
    {
        arch.view<rotation_c, angular_velocity_c>().each_n(begin, count, [dt](auto& rot, auto& avel) {
            rot.value += avel.value * dt;
        });
    }

    template<tavros::core::archetype_with<lifetime_c> T>
    inline void clear_dead(T& arch) noexcept
    {
        auto lt_v = arch.template get<lifetime_c>();
        for (size_t i = arch.size(); i-- > 0;) {
            if (lt_v[i].is_dead()) {
                arch.swap_and_pop(i);
            }
        }
    }

} // namespace tavros::particles

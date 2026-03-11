#pragma once

#include "particle_components.hpp"
#include <cmath>

namespace tavros::particles
{
    template<tavros::core::archetype_with<force_c, position_c, lifetime_c> T>
    inline void apply_attractor(T& arch, math::vec3 origin, float strength, float kill_radius, size_t begin, size_t count) noexcept
    {
        arch.template view<force_c, position_c, lifetime_c>()
            .each_n(begin, count, [&](auto& f, const auto& pos, auto& lt) {
                const math::vec3 delta = origin - pos.value;
                const float      dist_sq = math::squared_length(delta);

                if (kill_radius > 0.0f && dist_sq < kill_radius * kill_radius) {
                    lt.elapsed = lt.total;
                    return;
                }

                if (dist_sq < 1e-6f) {
                    return;
                }

                const float dist = std::sqrt(dist_sq);
                const float force_mag = strength / dist_sq;
                f.value += (delta / dist) * force_mag;
            });
    }

    template<tavros::core::archetype_with<force_c> T>
    inline void apply_wind(T& arch, math::vec3 wind_force, size_t begin, size_t count) noexcept
    {
        arch.template view<force_c>().each_n(begin, count, [&](auto& f) {
            f.value += wind_force;
        });
    }

    template<tavros::core::archetype_with<force_c> T, typename Rng>
    inline void apply_turbulence(T& arch, float strength, Rng&& rng) noexcept
    {
        arch.template view<force_c>().each([&](auto& f) {
            f.value.x += rng(-strength, strength);
            f.value.y += rng(-strength, strength);
            f.value.z += rng(-strength, strength);
        });
    }

    // -------------------------------------------------------------------------
    // Archetype-based effectors
    // -------------------------------------------------------------------------

    struct attractor_c
    {
        math::vec3 origin = {0.0f, 0.0f, 0.0f};
        float      strength = 10.0f;
        float      kill_radius = 0.2f;
    };

    struct wind_c
    {
        math::vec3 force = {0.0f, 0.0f, 0.0f};
    };

    using effector_archetype = tavros::core::basic_archetype<
        attractor_c,
        wind_c>;

    template<
        core::archetype_with<attractor_c, wind_c>             TEffectors,
        core::archetype_with<force_c, position_c, lifetime_c> TParticles>
    inline void apply_effectors(TEffectors& effectors, TParticles& particles, size_t begin, size_t count) noexcept
    {
        const auto attrs = effectors.get<attractor_c>();
        const auto winds = effectors.get<wind_c>();
        for (size_t i = 0; i < effectors.size(); ++i) {
            apply_attractor(particles, attrs[i].origin, attrs[i].strength, attrs[i].kill_radius, begin, count);
            // apply_wind(particles, winds[i].force, begin, count);
        }
    }

} // namespace tavros::particles
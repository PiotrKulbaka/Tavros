#pragma once

#include <tavros/core/memory/buffer_span.hpp>
#include "particle_components.hpp"

namespace tavros::particles
{

    template<tavros::core::archetype_with<position_c, color_c, size_c, lifetime_c, rotation_c> T>
    inline size_t fill_instances(T& particles, tavros::core::buffer_span<particle_instance> dst) noexcept
    {
        size_t count = 0;
        auto*  out = dst.begin();

        auto view = particles.view<const position_c, const color_c, const size_c, const lifetime_c, const rotation_c>();
        view.each([&](const auto& pos, const auto& col, const auto& sz, const auto& lt, const auto& rot) {
            const float t = lt.normalized();

            out->position[0] = pos.value.x;
            out->position[1] = pos.value.y;
            out->position[2] = pos.value.z;
            out->size = sz.sample(t);
            out->color = col.sample(t).color;
            out->rotation = rot.value;

            ++out;
            ++count;
        });

        return count;
    }

} // namespace tavros::particles

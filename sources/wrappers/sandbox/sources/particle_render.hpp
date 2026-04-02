#pragma once

#include <tavros/core/memory/buffer_span.hpp>
#include "particle_components.hpp"

namespace tavros::particles
{

    template<tavros::core::archetype_with<position_c, color0_c, size_c, lifetime_c, rotation_c> T>
    inline size_t fill_instances(T& particles, tavros::core::buffer_span<particle_instance> dst, size_t begin, size_t count) noexcept
    {
        size_t num = 0;
        auto*  out = &dst[begin];

        auto view = particles.view<const position_c, const computed_c, const rotation_c>();
        view.each_n(begin, count, [&](const auto& pos, const auto& comp, const auto& rot) {
            out->position[0] = pos.value.x;
            out->position[1] = pos.value.y;
            out->position[2] = pos.value.z;
            out->size = comp.size;
            out->color = comp.color;
            out->rotation = rot.value;

            ++out;
            ++num;
        });

        return count;
    }

} // namespace tavros::particles

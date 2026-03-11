#pragma once

#include <tavros/core/math/vec3.hpp>
#include <variant>
#include <cmath>

namespace tavros::particles
{
    enum class spawn_shape : uint8_t
    {
        point,
        rect,
        circle,
        sphere,
        sphere_shell,
    };

    struct spawn_shape_params
    {
        math::vec3 center;
        float      radius = 0.0f; // circle, sphere, sphere_shell
        float      half_x = 0.0f; // rect
        float      half_y = 0.0f; // rect
        float      half_z = 0.0f; // rect (0 = плоский)
    };

    template<typename Rng>
    [[nodiscard]] inline math::vec3 sample_shape(spawn_shape shape, const spawn_shape_params& p, Rng&& rng) noexcept
    {
        switch (shape) {
        case spawn_shape::point:
            return p.center;

        case spawn_shape::rect:
            return {
                p.center.x + rng(-p.half_x, p.half_x),
                p.center.y + rng(-p.half_y, p.half_y),
                p.center.z + rng(-p.half_z, p.half_z),
            };

        case spawn_shape::circle: {
            const float angle = rng(0.0f, 6.2831f);
            const float r = p.radius * std::sqrt(rng(0.0f, 1.0f));
            return {
                p.center.x + std::cos(angle) * r,
                p.center.y + std::sin(angle) * r,
                p.center.z,
            };
        }

        case spawn_shape::sphere: {
            const float az = rng(-3.14159f, 3.14159f);
            const float el = std::asin(rng(-1.0f, 1.0f));
            const float r = p.radius * std::cbrt(rng(0.0f, 1.0f));
            return {
                p.center.x + r * std::cos(el) * std::cos(az),
                p.center.y + r * std::cos(el) * std::sin(az),
                p.center.z + r * std::sin(el),
            };
        }

        case spawn_shape::sphere_shell: {
            const float az = rng(-3.14159f, 3.14159f);
            const float el = std::asin(rng(-1.0f, 1.0f));
            return {
                p.center.x + p.radius * std::cos(el) * std::cos(az),
                p.center.y + p.radius * std::cos(el) * std::sin(az),
                p.center.z + p.radius * std::sin(el),
            };
        }
        }

        return p.center;
    }

} // namespace tavros::particles
#include <tavros/core/geometry/plane.hpp>

namespace tavros::geometry
{

    static_assert(sizeof(plane) == 16, "incorrect size");
    static_assert(alignof(plane) == 4, "incorrect alignment");

    inline plane plane::from_normal_point(const math::vec3& normal, const math::vec3& point) noexcept
    {
        return plane(normal, -math::dot(normal, point));
    }

    inline plane plane::from_points(const math::vec3& a, const math::vec3& b, const math::vec3& c) noexcept
    {
        const auto ab = b - a;
        const auto ac = c - a;
        auto       normal = math::normalize(math::cross(ac, ab));
        auto       d = -math::dot(normal, a);
        return plane(normal, d);
    }

    inline constexpr plane::plane() noexcept
        : normal(0.0, 0.0, 1.0)
        , d(0.0)
    {
    }

    inline constexpr plane::plane(const math::vec3& normal, float d) noexcept
        : normal(normal)
        , d(d)
    {
    }

} // namespace tavros::geometry

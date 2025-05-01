#include <tavros/core/geometry/sphere.hpp>

namespace tavros::geometry
{

    static_assert(sizeof(sphere) == 16, "incorrect size");
    static_assert(alignof(sphere) == 4, "incorrect alignment");

    inline constexpr sphere::sphere() noexcept
        : center(0.0f, 0.0f, 0.0f)
        , radius(0.0f)
    {
    }

    inline constexpr sphere::sphere(const math::vec3& center, float radius) noexcept
        : center(center)
        , radius(radius)
    {
    }

} // namespace tavros::geometry

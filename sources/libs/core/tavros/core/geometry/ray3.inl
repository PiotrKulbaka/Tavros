#include <tavros/core/geometry/ray3.hpp>

namespace tavros::geometry
{

    inline constexpr ray3::ray3() noexcept
        : origin(0.0f, 0.0f, 0.0f)
        , direction(0.0f, 0.0f, 1.0f)
    {
    }

    inline constexpr ray3::ray3(const math::vec3& origin, const math::vec3& direction) noexcept
        : origin(origin)
        , direction(direction)
    {
    }

    inline constexpr math::vec3 ray3::at(float distance) const noexcept
    {
        return origin + direction * distance;
    }

} // namespace tavros::geometry

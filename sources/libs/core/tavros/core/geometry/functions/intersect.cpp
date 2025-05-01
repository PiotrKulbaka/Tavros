#include <tavros/core/geometry/functions/intersect.hpp>

#include <tavros/core/geometry/plane.hpp>
#include <tavros/core/geometry/sphere.hpp>
#include <tavros/core/geometry/ray3.hpp>

namespace tavros::geometry
{

    float intersect(const ray3& ray, const plane& plane) noexcept
    {
        auto denom = math::dot(ray.direction, plane.normal);
        if (math::almost_zero(denom)) {
            return 0.0f;
        }
        auto t = -(math::dot(plane.normal, ray.origin) + plane.d) / denom;
        return t < 0.0f ? 0.0f : t;
    }

    float intersect(const ray3& ray, const sphere& sphere) noexcept
    {
        const auto radius_sq = sphere.radius * sphere.radius;
        const auto to_center = sphere.center - ray.origin;
        const auto dist_sq = math::squared_length(to_center);

        const auto projection = math::dot(to_center, ray.direction); // assume ray.direction already normalized

        const auto delta_sq = radius_sq - (dist_sq - projection * projection);
        if (delta_sq < 0.0f) {
            return 0.0f;
        }

        const auto delta = math::sqrt(delta_sq);
        auto       t = (dist_sq < radius_sq) ? (projection + delta) : (projection - delta);

        return t < 0.0f ? 0.0f : t;
    }

} // namespace tavros::geometry


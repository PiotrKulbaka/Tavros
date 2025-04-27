#include <tavros/core/geometry/functions/intersect.hpp>

#include <tavros/core/geometry/plane.hpp>
#include <tavros/core/geometry/ray3.hpp>

namespace tavros::geometry
{

    float intersect(const ray3& ray, const plane& plane)
    {
        auto denom = math::dot(ray.direction, plane.normal);
        if (math::almost_zero(denom)) {
            return 0.0f;
        }
        auto t = -(math::dot(plane.normal, ray.origin) + plane.d) / denom;
        return t < 0.0f ? 0.0f : t;
    }

} // namespace tavros::geometry


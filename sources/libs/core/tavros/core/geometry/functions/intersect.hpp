#pragma once

/**
 * @file intersect.hpp
 */

#include <tavros/core/math.hpp>

namespace tavros::geometry
{
    class plane;
    class sphere;
    class ray3;
    class aabb2;

    float intersect(const ray3& ray, const plane& plane) noexcept;

    float intersect(const ray3& ray, const sphere& sphere) noexcept;

    bool intersects(const aabb2& box1, const aabb2& box2) noexcept;

} // namespace tavros::geometry


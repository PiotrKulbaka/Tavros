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


    float intersect(const ray3& ray, const plane& plane) noexcept;

    float intersect(const ray3& ray, const sphere& sphere) noexcept;

} // namespace tavros::geometry


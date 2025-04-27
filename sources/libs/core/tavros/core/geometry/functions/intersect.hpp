#pragma once

/**
 * @file intersect.hpp
 */

#include <tavros/core/math.hpp>

namespace tavros::geometry
{
    class plane;
    class ray3;


    float intersect(const ray3& ray, const plane& plane);

} // namespace tavros::geometry


#pragma once

#include <tavros/core/geometry/aabb2.hpp>

namespace tavros::renderer
{

    struct rect_layout_c : public geometry::aabb2
    {
        constexpr rect_layout_c& operator=(const geometry::aabb2& other) noexcept
        {
            geometry::aabb2::operator=(other);
            return *this;
        }
    };

    struct aabb2_c : public geometry::aabb2
    {
        constexpr aabb2_c& operator=(const geometry::aabb2& other) noexcept
        {
            geometry::aabb2::operator=(other);
            return *this;
        }
    };

} // namespace tavros::renderer

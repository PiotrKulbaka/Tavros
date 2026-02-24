#pragma once

#include <tavros/core/math/vec2.hpp>

namespace tavros::renderer
{

    struct pos2_c : public math::vec2
    {
        constexpr pos2_c& operator=(const math::vec2& other) noexcept
        {
            math::vec2::operator=(other);
            return *this;
        }
    };

} // namespace tavros::renderer
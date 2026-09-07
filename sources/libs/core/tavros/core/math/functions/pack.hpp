#pragma once

#include <tavros/core/math/functions/basic_math.hpp>
#include <tavros/core/math/vec2.hpp>
#include <fhf/fhf.hpp>

namespace tavros::math
{

    inline uint16 pack_snorm16(float v) noexcept
    {
        v = std::clamp(v, -1.0f, 1.0f);
        int16 s = static_cast<int16>(std::lround(v * 32767.0f));
        return static_cast<uint16>(s);
    }

    inline uint32 pack_snorm2x16(float x, float y) noexcept
    {
        int16 sx = pack_snorm16(x);
        int16 sy = pack_snorm16(y);
        return (static_cast<uint32>(sy) << 16) | (static_cast<uint32>(sx) & 0xFFFFu);
    }

    inline uint16 float_to_half(float value) noexcept
    {
        return fhf::pack(value);
    }

    inline uint32 pack_half2x16(float x, float y) noexcept
    {
        uint32 hx = float_to_half(x);
        uint32 hy = float_to_half(y);
        return hx | (hy << 16);
    }

    inline uint32 pack_half2x16(const math::vec2& v) noexcept
    {
        return pack_half2x16(v.x, v.y);
    }

} // namespace tavros::math

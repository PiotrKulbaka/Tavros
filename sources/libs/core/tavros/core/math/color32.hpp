#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/math/vec4.hpp>

namespace tavros::math
{

    /**
     * @brief 8-bit per channel RGBA color packed into 32 bits.
     *
     * Provides simple utilities for manipulating 8-bit color values, converting
     * from floating-point colors, linear interpolation, and checking transparency.
     */
    class color32
    {
    public:
        /**
         * @brief Default-initializes the color to 0x00000000.
         */
        constexpr color32() noexcept
            : packed(0)
        {
        }

        /**
         * @brief Initializes the color from individual 8-bit channels.
         *
         * @param r Red channel.
         * @param g Green channel.
         * @param b Blue channel.
         * @param a Alpha channel (defaults to 255).
         */
        constexpr color32(uint8 r, uint8 g, uint8 b, uint8 a = 255) noexcept
            : r(r)
            , g(g)
            , b(b)
            , a(a)
        {
        }

        /**
         * @brief Initializes the color from uint32 color.
         *
         * @param rgba Red Green Blue and Alpha channels.
         */
        constexpr color32(uint32 rgba) noexcept
            : packed(rgba)
        {
        }

        /**
         * @brief Converts a floating-point vec4 color to 8-bit RGBA.
         *
         * @param color Input color where each component is in [0.0, 1.0].
         */
        constexpr color32(const vec4& color) noexcept
            : r(static_cast<uint8>(color.r * 255.0f))
            , g(static_cast<uint8>(color.g * 255.0f))
            , b(static_cast<uint8>(color.b * 255.0f))
            , a(static_cast<uint8>(color.a * 255.0f))
        {
        }

        /**
         * @brief Sets the color from 8-bit channels (packed into uint32).
         */
        constexpr void set(uint8 r, uint8 g, uint8 b, uint8 a) noexcept
        {
            packed = (static_cast<uint32>(r) << 24) | (static_cast<uint32>(g) << 16) | (static_cast<uint32>(b) << 8) | (static_cast<uint32>(a) << 0);
        }

        /**
         * @brief Sets the color from vec4.
         */
        constexpr void set(const vec4& color) noexcept
        {
            r = static_cast<uint8>(color.r * 255.0f);
            g = static_cast<uint8>(color.g * 255.0f);
            b = static_cast<uint8>(color.b * 255.0f);
            a = static_cast<uint8>(color.a * 255.0f);
        }

        /**
         * @brief Returns true if alpha == 0.
         */
        constexpr bool is_transparent() const noexcept
        {
            return a == 0;
        }

        /**
         * @brief Returns true if alpha == 255.
         */
        constexpr bool is_opaque() const noexcept
        {
            return a == 255;
        }

        /**
         * @brief Linearly interpolates between two colors.
         *
         * @param c1 First color.
         * @param c2 Second color.
         * @param t Interpolation factor in [0, 1].
         */
        static constexpr color32 lerp(const color32& c1, const color32& c2, float t)
        {
            const int32 ti = static_cast<int32>(t * 256.0f);
            return color32(
                static_cast<uint8>(static_cast<int32>(c1.r) + ((static_cast<int32>(c2.r) - static_cast<int32>(c1.r)) * static_cast<int32>(ti) >> 8)),
                static_cast<uint8>(static_cast<int32>(c1.g) + ((static_cast<int32>(c2.g) - static_cast<int32>(c1.g)) * static_cast<int32>(ti) >> 8)),
                static_cast<uint8>(static_cast<int32>(c1.b) + ((static_cast<int32>(c2.b) - static_cast<int32>(c1.b)) * static_cast<int32>(ti) >> 8)),
                static_cast<uint8>(static_cast<int32>(c1.a) + ((static_cast<int32>(c2.a) - static_cast<int32>(c1.a)) * static_cast<int32>(ti) >> 8))
            );
        }

    public:
        union
        {
            struct
            {
                uint8 r, g, b, a;
            };
            uint32 packed; /// Packed RGBA value in 0xAABBGGRR format.
        };
    };

    static_assert(sizeof(color32) == 4);

} // namespace tavros::math

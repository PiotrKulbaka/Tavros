#pragma once

#include <tavros/core/types.hpp>

namespace tavros::math
{

    /**
     * @brief Counts the number of set bits (1s) in a 64-bit integer.
     * @param x Input value.
     * @return Number of bits set to 1.
     * @note Useful for counting active slots, flags, or resources.
     */
    inline size_t bit_count(uint64 x) noexcept
    {
#if defined(_MSC_VER)
        return static_cast<size_t>(__popcnt64(x));
#else
        return static_cast<size_t>(__builtin_popcountll(x));
#endif
    }

    /**
     * @brief Checks if a value is a power of two.
     * @param x Input value.
     * @return true if x is a power of two, false otherwise.
     */
    inline constexpr bool is_power_of_two(uint64 x) noexcept
    {
        return x != 0 && (x & (x - 1)) == 0;
    }

    /**
     * @brief Rounds up to the nearest power of two.
     * @param x Input value.
     * @return Smallest power of two >= x.
     */
    inline constexpr uint64 ceil_power_of_two(uint64 x) noexcept
    {
        if (x == 0) {
            return 1;
        }
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x |= x >> 32;
        return x + 1;
    }

    /**
     * @brief Rounds down to the nearest power of two.
     * @param x Input value.
     * @return Largest power of two <= x.
     */
    inline constexpr uint64 floor_power_of_two(uint64 x) noexcept
    {
        if (x == 0) {
            return 0;
        }
        uint64 y = ceil_power_of_two(x);
        return (y > x) ? y >> 1 : y;
    }

    /**
     * @brief Returns index of first set bit (1), starting from LSB.
     * @param x Input value.
     * @return Zero-based position of first set bit, or 64 if no bits set.
     */
    inline size_t first_set_bit(uint64 x) noexcept
    {
#if defined(_MSC_VER)
        unsigned long idx;
        if (_BitScanForward64(&idx, x)) {
            return static_cast<size_t>(idx);
        }
        return 64;
#else
        return x ? static_cast<size_t>(__builtin_ffsll(x) - 1) : 64;
#endif
    }

    /**
     * @brief Returns index of first zero bit (0), starting from LSB.
     * @param x Input value.
     * @return Zero-based position of first zero bit, or 64 if all bits set.
     */
    inline size_t first_zero_bit(uint64 x) noexcept
    {
        return first_set_bit(~x);
    }

    /**
     * @brief Returns index of highest set bit (MSB).
     * @param x Input value.
     * @return Zero-based position of highest set bit, or 64 if no bits set.
     */
    inline size_t highest_set_bit(uint64 x) noexcept
    {
#if defined(_MSC_VER)
        unsigned long idx;
        if (_BitScanReverse64(&idx, x)) {
            return static_cast<size_t>(idx);
        }
        return 64;
#else
        return x ? 63 - __builtin_clzll(x) : 64;
#endif
    }

    /**
     * @brief Counts leading zeros (zeros before first set bit from MSB).
     * @param x Input value.
     * @return Number of leading zeros.
     */
    inline size_t count_leading_zeros(uint64 x) noexcept
    {
#if defined(_MSC_VER)
        unsigned long idx;
        if (_BitScanReverse64(&idx, x)) {
            return 63 - idx;
        }
        return 64;
#else
        return x ? static_cast<size_t>(__builtin_clzll(x)) : 64;
#endif
    }

    /**
     * @brief Counts trailing zeros (zeros after first set bit from LSB).
     * @param x Input value.
     * @return Number of trailing zeros.
     */
    inline size_t count_trailing_zeros(uint64 x) noexcept
    {
#if defined(_MSC_VER)
        unsigned long idx;
        if (_BitScanForward64(&idx, x)) {
            return static_cast<size_t>(idx);
        }
        return 64;
#else
        return x ? static_cast<size_t>(__builtin_ctzll(x)) : 64;
#endif
    }

    inline size_t align_up(size_t value, size_t align) noexcept
    {
        TAV_ASSERT(is_power_of_two(align)); // align must be power of two
        return (value + (align - 1)) & ~(align - 1);
    }

} // namespace tavros::math

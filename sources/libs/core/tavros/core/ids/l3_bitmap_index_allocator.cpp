#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/math/bitops.hpp>

namespace tavros::core
{

    index_type l3_bitmap_index_allocator::allocate() noexcept
    {
        const size_t l1_base_index = 0; // Always 0
        const uint64 l1_word = m_l1_map[l1_base_index];
        if (l1_word != UINT64_MAX) {
            const size_t l1_free_bit = math::first_zero_bit(l1_word);

            const size_t l2_base_index = l1_base_index * k_bits_per_word + l1_free_bit;
            const uint64 l2_word = m_l2_map[l2_base_index];
            if (l2_word != UINT64_MAX) {
                const size_t l2_free_bit = math::first_zero_bit(l2_word);

                const size_t l3_base_index = l2_base_index * k_bits_per_word + l2_free_bit;
                const uint64 l3_word = m_l3_map[l3_base_index];
                if (l3_word != UINT64_MAX) {
                    const size_t l3_free_bit = math::first_zero_bit(l3_word);
                    const size_t index = l3_base_index * k_bits_per_word + l3_free_bit;

                    TAV_ASSERT(index < k_max_index);

                    --m_remaining;

                    // Update l3 map
                    m_l3_map[l3_base_index] |= (1ull << l3_free_bit);

                    const uint64 l3_word_now = m_l3_map[l3_base_index];
                    if (l3_word_now == UINT64_MAX) {
                        // Update l2 map
                        m_l2_map[l2_base_index] |= (1ull << l2_free_bit);

                        const uint64 l2_word_now = m_l2_map[l2_base_index];
                        if (l2_word_now == UINT64_MAX) {
                            // Update l1 map
                            m_l1_map[l1_base_index] |= (1ull << l1_free_bit);
                        }
                    }

                    return static_cast<index_type>(index);
                }
            }
        }

        return invalid_index;
    }

    void l3_bitmap_index_allocator::deallocate(index_type index) noexcept
    {
        TAV_ASSERT(index < k_max_index);
        auto deallocated = try_deallocate(index);
        TAV_ASSERT(deallocated);
        (void) deallocated;
    }

    bool l3_bitmap_index_allocator::try_deallocate(index_type index) noexcept
    {
        if (index >= k_max_index) {
            return false;
        }

        // NOTE: Since k_bits_per_word == 64 (a power of two), the compiler will
        // automatically optimize division and modulo operations into fast bitwise
        // instructions:
        //   index / 64  -> index >> 6
        //   index % 64  -> index & 63
        // No manual replacement is required.
        const size_t l3_bit_idx = index % k_bits_per_word;
        const size_t l3_base_index = index / k_bits_per_word;

        const uint64 l3_word = m_l3_map[l3_base_index];

        if (0 == (l3_word & (1ull << l3_bit_idx))) {
            // Index is not allocated
            return false;
        }

        ++m_remaining;

        TAV_ASSERT(m_remaining <= k_max_index);

        m_l3_map[l3_base_index] &= ~(1ull << l3_bit_idx);
        if (l3_word == UINT64_MAX) {
            const size_t l2_bit_idx = l3_base_index % k_bits_per_word;
            const size_t l2_base_index = l3_base_index / k_bits_per_word;

            const uint64 l2_word = m_l2_map[l2_base_index];
            m_l2_map[l2_base_index] &= ~(1ull << l2_bit_idx);

            if (l2_word == UINT64_MAX) {
                const size_t l1_bit_idx = l2_base_index % k_bits_per_word;
                const size_t l1_base_index = l2_base_index / k_bits_per_word;

                m_l1_map[l1_base_index] &= ~(1ull << l1_bit_idx);
            }
        }

        return true;
    }

    bool l3_bitmap_index_allocator::allocated(index_type index) noexcept
    {
        if (index >= k_max_index) {
            return false;
        }

        const size_t l3_bit_idx = index % k_bits_per_word;
        const size_t l3_base_index = index / k_bits_per_word;

        const uint64 l3_word = m_l3_map[l3_base_index];

        return static_cast<bool>(l3_word & (1ull << l3_bit_idx));
    }

    void l3_bitmap_index_allocator::reset() noexcept
    {
        memset(m_l1_map, 0, sizeof(m_l1_map));
        memset(m_l2_map, 0, sizeof(m_l2_map));
        memset(m_l3_map, 0, sizeof(m_l3_map));

        m_remaining = k_max_index;
    }

    size_t l3_bitmap_index_allocator::max_index() const noexcept
    {
        return k_max_index;
    }

    size_t l3_bitmap_index_allocator::remaining() const noexcept
    {
        return m_remaining;
    }

} // namespace tavros::core

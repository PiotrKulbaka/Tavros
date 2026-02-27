#include <tavros/core/ids/l2_bitmap_index_allocator.hpp>

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/math/bitops.hpp>

namespace tavros::core
{

    l2_bitmap_index_allocator::l2_bitmap_index_allocator() noexcept
    {
        reset();
    }

    index_t l2_bitmap_index_allocator::allocate() noexcept
    {
        const size_t l1_base_index = 0; // Always 0
        const uint64 l1_word = m_l1_map[l1_base_index];
        if (l1_word != UINT64_MAX) {
            const size_t l1_free_bit = math::first_zero_bit(l1_word);

            const size_t l2_base_index = l1_base_index * k_bits_per_word + l1_free_bit;
            const uint64 l2_word = m_l2_map[l2_base_index];
            if (l2_word != UINT64_MAX) {
                const size_t l2_free_bit = math::first_zero_bit(l2_word);
                const size_t index = l2_base_index * k_bits_per_word + l2_free_bit;

                TAV_ASSERT(index < k_max_index);

                ++m_size;

                // Update l2 map
                m_l2_map[l2_base_index] |= (1ull << l2_free_bit);

                const uint64 l2_word_now = m_l2_map[l2_base_index];
                if (l2_word_now == UINT64_MAX) {
                    // Update l1 map
                    m_l1_map[l1_base_index] |= (1ull << l1_free_bit);
                }

                return static_cast<index_t>(index);
            }
        }

        return invalid_index;
    }

    bool l2_bitmap_index_allocator::deallocate(index_t index) noexcept
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
        const size_t l2_bit_idx = index % k_bits_per_word;
        const size_t l2_base_index = index / k_bits_per_word;

        const uint64 l2_word = m_l2_map[l2_base_index];

        if (0 == (l2_word & (1ull << l2_bit_idx))) {
            // Index is not allocated
            return false;
        }

        --m_size;

        TAV_ASSERT(m_size < k_max_index);

        m_l2_map[l2_base_index] &= ~(1ull << l2_bit_idx);
        if (l2_word == UINT64_MAX) {
            const size_t l1_bit_idx = l2_base_index % k_bits_per_word;
            const size_t l1_base_index = l2_base_index / k_bits_per_word;

            m_l1_map[l1_base_index] &= ~(1ull << l1_bit_idx);
        }

        return true;
    }

    bool l2_bitmap_index_allocator::contains(index_t index) const noexcept
    {
        if (index >= k_max_index) {
            return false;
        }

        const size_t l2_bit_idx = index % k_bits_per_word;
        const size_t l2_base_index = index / k_bits_per_word;

        const uint64 l2_word = m_l2_map[l2_base_index];

        return static_cast<bool>(l2_word & (1ull << l2_bit_idx));
    }

    void l2_bitmap_index_allocator::reset() noexcept
    {
        memset(m_l1_map, 0, sizeof(m_l1_map));
        memset(m_l2_map, 0, sizeof(m_l2_map));
        m_size = 0;
    }

    size_t l2_bitmap_index_allocator::capacity() const noexcept
    {
        return k_max_index;
    }

    size_t l2_bitmap_index_allocator::size() const noexcept
    {
        return m_size;
    }

} // namespace tavros::core

#include <tavros/core/ids/l2_index_allocator.hpp>

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/math/bitops.hpp>

namespace tavros::core
{

    index_allocator_base::index_t l2_index_allocator::allocate() noexcept
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

                TAV_ASSERT(index < k_total_indices);

                --m_remaining_indices;

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

    void l2_index_allocator::deallocate(index_t index) noexcept
    {
        TAV_ASSERT(index < k_total_indices);

        if (index >= k_total_indices) {
            return;
        }

        // NOTE: Since k_bits_per_word == 64 (a power of two), the compiler will
        // automatically optimize division and modulo operations into fast bitwise
        // instructions:
        //   index / 64  -> index >> 6
        //   index % 64  -> index & 63
        // No manual replacement is required.
        const size_t l2_bit_idx = index % k_bits_per_word;
        const size_t l2_base_index = index / k_bits_per_word;

        const size_t l1_bit_idx = l2_base_index % k_bits_per_word;
        const size_t l1_base_index = l2_base_index / k_bits_per_word;

        const uint64 l2_word = m_l2_map[l2_base_index];

        TAV_ASSERT(0 != (l2_word & (1ull << l2_bit_idx)));

        ++m_remaining_indices;

        TAV_ASSERT(m_remaining_indices <= k_total_indices);

        m_l2_map[l2_base_index] &= ~(1ull << l2_bit_idx);
        if (l2_word == UINT64_MAX) {
            m_l1_map[l1_base_index] &= ~(1ull << l1_bit_idx);
        }
    }

    void l2_index_allocator::reset() noexcept
    {
        memset(m_l1_map, 0, sizeof(m_l1_map));
        memset(m_l2_map, 0, sizeof(m_l2_map));

        m_remaining_indices = k_total_indices;
    }

    size_t l2_index_allocator::capacity() const noexcept
    {
        return k_total_indices;
    }

    size_t l2_index_allocator::remaining() const noexcept
    {
        return m_remaining_indices;
    }

} // namespace tavros::core

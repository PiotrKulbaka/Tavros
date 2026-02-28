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
        const uint64 l1_word = m_l1_map[0];
        if (l1_word == UINT64_MAX) {
            return invalid_index;
        }

        const size_t l1_free_bit = math::first_zero_bit(l1_word);
        const size_t l2_base_index = l1_free_bit;
        const uint64 l2_word = m_l2_map[l2_base_index];
        if (l2_word == UINT64_MAX) {
            return invalid_index;
        }

        const size_t l2_free_bit = math::first_zero_bit(l2_word);
        const size_t index = l2_base_index * k_bits_per_word + l2_free_bit;

        TAV_ASSERT(index < capacity());

        ++m_size;

        m_l2_map[l2_base_index] |= (1ull << l2_free_bit);
        if (m_l2_map[l2_base_index] == UINT64_MAX) {
            m_l1_map[0] |= (1ull << l1_free_bit);
        }

        return static_cast<index_t>(index);
    }

    bool l2_bitmap_index_allocator::deallocate(index_t index) noexcept
    {
        if (index >= capacity()) {
            return false;
        }

        // NOTE: Since k_bits_per_word == 64 (a power of two), the compiler will
        // automatically optimize division and modulo operations into fast bitwise
        // instructions:
        //   index / 64  -> index >> 6
        //   index % 64  -> index & 63
        const size_t l2_bit_idx = index % k_bits_per_word;
        const size_t l2_base_index = index / k_bits_per_word;

        const uint64 l2_word = m_l2_map[l2_base_index];

        if (0 == (l2_word & (1ull << l2_bit_idx))) {
            // Index is not allocated
            return false;
        }

        --m_size;

        TAV_ASSERT(m_size < capacity());

        m_l2_map[l2_base_index] &= ~(1ull << l2_bit_idx);

        // If the block was full before, clear the l1 bit
        if (l2_word == UINT64_MAX) {
            const size_t l1_bit_idx = l2_base_index % k_bits_per_word;
            const size_t l1_base_index = l2_base_index / k_bits_per_word;

            m_l1_map[l1_base_index] &= ~(1ull << l1_bit_idx);
        }

        return true;
    }

    bool l2_bitmap_index_allocator::contains(index_t index) const noexcept
    {
        if (index >= capacity()) {
            return false;
        }

        const size_t l2_bit_idx = index % k_bits_per_word;
        const size_t l2_base_index = index / k_bits_per_word;

        const uint64 l2_word = m_l2_map[l2_base_index];

        return static_cast<bool>(l2_word & (1ull << l2_bit_idx));
    }

    void l2_bitmap_index_allocator::reset() noexcept
    {
        std::memset(m_l1_map, 0, sizeof(m_l1_map));
        std::memset(m_l2_map, 0, sizeof(m_l2_map));
        m_size = 0;
    }

    size_t l2_bitmap_index_allocator::capacity() const noexcept
    {
        return k_capacity;
    }

    size_t l2_bitmap_index_allocator::size() const noexcept
    {
        return m_size;
    }

    index_t l2_bitmap_index_allocator::find_first() const noexcept
    {
        if (m_size == 0) {
            return static_cast<index_t>(capacity());
        }

        for (size_t i = 0; i < k_l2_map_size; ++i) {
            if (m_l2_map[i] != 0) {
                return static_cast<index_t>(i * k_bits_per_word + math::first_set_bit(m_l2_map[i]));
            }
        }

        return static_cast<index_t>(capacity());
    }

    index_t l2_bitmap_index_allocator::next_after(index_t pos) const noexcept
    {
        if (pos >= capacity()) {
            return static_cast<index_t>(capacity());
        }

        ++pos;

        while (pos < capacity()) {
            const size_t l2_idx = pos / k_bits_per_word;
            const size_t l2_bit = pos % k_bits_per_word;

            const uint64 l2_word = m_l2_map[l2_idx] >> l2_bit;
            if (l2_word != 0) {
                return static_cast<index_t>(l2_idx * k_bits_per_word + l2_bit + math::first_set_bit(l2_word));
            }

            const size_t next_l2 = l2_idx + 1;
            if (next_l2 >= k_l2_map_size) {
                break;
            }

            // Invert l1: bit set means block is NOT full (may still be empty, but worth checking)
            const uint64 l1_remaining = ~m_l1_map[0] >> next_l2;
            if (l1_remaining == 0) {
                break;
            }

            const size_t jump_l2 = next_l2 + math::first_set_bit(l1_remaining);
            if (jump_l2 >= k_l2_map_size) {
                break;
            }

            pos = static_cast<index_t>(jump_l2 * k_bits_per_word);
        }

        return static_cast<index_t>(capacity());
    }

} // namespace tavros::core

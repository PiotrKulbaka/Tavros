#include <tavros/core/ids/l3_bitmap_index_allocator.hpp>

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/math/bitops.hpp>

namespace tavros::core
{

    l3_bitmap_index_allocator::l3_bitmap_index_allocator() noexcept
    {
        reset();
    }

    index_t l3_bitmap_index_allocator::allocate() noexcept
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
        const size_t l3_base_index = l2_base_index * k_bits_per_word + l2_free_bit;
        const uint64 l3_word = m_l3_map[l3_base_index];
        if (l3_word == UINT64_MAX) {
            return invalid_index;
        }

        const size_t l3_free_bit = math::first_zero_bit(l3_word);
        const size_t index = l3_base_index * k_bits_per_word + l3_free_bit;

        TAV_ASSERT(index < capacity());

        ++m_size;

        m_l3_map[l3_base_index] |= (1ull << l3_free_bit);
        if (m_l3_map[l3_base_index] == UINT64_MAX) {
            m_l2_map[l2_base_index] |= (1ull << l2_free_bit);
            if (m_l2_map[l2_base_index] == UINT64_MAX) {
                m_l1_map[0] |= (1ull << l1_free_bit);
            }
        }

        return static_cast<index_t>(index);
    }

    bool l3_bitmap_index_allocator::deallocate(index_t index) noexcept
    {
        if (index >= capacity()) {
            return false;
        }

        // NOTE: Since k_bits_per_word == 64 (a power of two), the compiler
        // optimizes division and modulo into shift and mask automatically:
        //   index / 64  ->  index >> 6
        //   index % 64  ->  index & 63
        const size_t l3_bit_idx = index % k_bits_per_word;
        const size_t l3_base_index = index / k_bits_per_word;
        const uint64 l3_word = m_l3_map[l3_base_index];

        if ((l3_word & (1ull << l3_bit_idx)) == 0) {
            return false;
        }

        --m_size;
        TAV_ASSERT(m_size < capacity());

        m_l3_map[l3_base_index] &= ~(1ull << l3_bit_idx);

        if (l3_word != UINT64_MAX) {
            return true;
        }

        const size_t l2_bit_idx = l3_base_index % k_bits_per_word;
        const size_t l2_base_index = l3_base_index / k_bits_per_word;
        const uint64 l2_word = m_l2_map[l2_base_index];

        m_l2_map[l2_base_index] &= ~(1ull << l2_bit_idx);

        if (l2_word != UINT64_MAX) {
            return true;
        }

        const size_t l1_bit_idx = l2_base_index % k_bits_per_word;
        m_l1_map[0] &= ~(1ull << l1_bit_idx);

        return true;
    }

    bool l3_bitmap_index_allocator::contains(index_t index) const noexcept
    {
        if (index >= capacity()) {
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
        m_size = 0;
    }

    size_t l3_bitmap_index_allocator::capacity() const noexcept
    {
        return k_capacity;
    }

    size_t l3_bitmap_index_allocator::size() const noexcept
    {
        return m_size;
    }

    index_t l3_bitmap_index_allocator::find_first() const noexcept
    {
        if (m_size == 0) {
            return static_cast<index_t>(capacity());
        }

        // Use l1 to find the first non-full l2 block,
        // then l2 to find the first non-full l3 block
        const uint64 l1_word = ~m_l1_map[0]; // invert: 1 = not full = potentially non-empty
        if (l1_word == 0) {
            return static_cast<index_t>(capacity());
        }

        const size_t l2_idx = math::first_set_bit(l1_word);
        const uint64 l2_word = ~m_l2_map[l2_idx]; // invert: 1 = not full = potentially non-empty
        if (l2_word == 0) {
            return static_cast<index_t>(capacity());
        }

        const size_t l3_idx = l2_idx * k_bits_per_word + math::first_set_bit(l2_word);
        const uint64 l3_word = m_l3_map[l3_idx];
        if (l3_word == 0) {
            return static_cast<index_t>(capacity());
        }

        return static_cast<index_t>(l3_idx * k_bits_per_word + math::first_set_bit(l3_word));
    }

    index_t l3_bitmap_index_allocator::next_after(index_t pos) const noexcept
    {
        if (pos >= capacity()) {
            return static_cast<index_t>(capacity());
        }

        ++pos;

        while (pos < capacity()) {
            const size_t l3_idx = pos / k_bits_per_word;
            const size_t l3_bit = pos % k_bits_per_word;

            // Mask already-visited bits, check for next allocated bit in this word
            const uint64 l3_word = m_l3_map[l3_idx] >> l3_bit;
            if (l3_word != 0) {
                return static_cast<index_t>(l3_idx * k_bits_per_word + l3_bit + math::first_set_bit(l3_word));
            }

            // Current l3 block exhausted — try next l3 block in the same l2 group
            const size_t next_l3 = l3_idx + 1;
            const size_t cur_l2 = l3_idx / k_bits_per_word;
            const size_t end_l3 = (cur_l2 + 1) * k_bits_per_word; // first l3 of next l2 group

            if (next_l3 < end_l3 && next_l3 < k_l3_map_size) {
                // Still inside the same l2 group — scan remaining l3 blocks via l2 bitmap
                const size_t l3_bit_in_l2 = next_l3 % k_bits_per_word;
                const uint64 l2_remaining = ~m_l2_map[cur_l2] >> l3_bit_in_l2;
                if (l2_remaining != 0) {
                    // Jump to the next non-full l3 block within this l2 group
                    const size_t jump_l3 = next_l3 + math::first_set_bit(l2_remaining);
                    if (jump_l3 < k_l3_map_size) {
                        const uint64 jump_word = m_l3_map[jump_l3];
                        if (jump_word != 0) {
                            return static_cast<index_t>(jump_l3 * k_bits_per_word + math::first_set_bit(jump_word));
                        }
                        pos = static_cast<index_t>(jump_l3 * k_bits_per_word);
                        continue;
                    }
                }
            }

            // Current l2 group exhausted — jump to the next l2 group via l1
            const size_t next_l2 = cur_l2 + 1;
            if (next_l2 >= k_l2_map_size) {
                break;
            }

            const uint64 l1_remaining = ~m_l1_map[0] >> next_l2;
            if (l1_remaining == 0) {
                break;
            }

            const size_t jump_l2 = next_l2 + math::first_set_bit(l1_remaining);
            if (jump_l2 >= k_l2_map_size) {
                break;
            }

            // Within the jumped l2 group, find the first non-full l3 block
            const uint64 l2_word = ~m_l2_map[jump_l2];
            if (l2_word == 0) {
                // All l3 blocks in this group are full — shouldn't happen if m_size is correct,
                // but handle gracefully by moving to the start of next group
                pos = static_cast<index_t>((jump_l2 + 1) * k_bits_per_word * k_bits_per_word);
                continue;
            }

            const size_t jump_l3 = jump_l2 * k_bits_per_word + math::first_set_bit(l2_word);
            if (jump_l3 >= k_l3_map_size) {
                break;
            }

            pos = static_cast<index_t>(jump_l3 * k_bits_per_word);
        }

        return static_cast<index_t>(capacity());
    }

} // namespace tavros::core

#include <tavros/core/ids/l4_bitmap_index_allocator.hpp>

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/math/bitops.hpp>

namespace tavros::core
{

    l4_bitmap_index_allocator::l4_bitmap_index_allocator() noexcept
    {
        reset();
    }

    index_t l4_bitmap_index_allocator::allocate() noexcept
    {
        const size_t l1_base_index = 0;
        const uint64 l1_word = m_l1_map[l1_base_index];
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
        const size_t l4_base_index = l3_base_index * k_bits_per_word + l3_free_bit;
        const uint64 l4_word = m_l4_map[l4_base_index];
        if (l4_word == UINT64_MAX) {
            return invalid_index;
        }

        const size_t l4_free_bit = math::first_zero_bit(l4_word);
        const size_t index = l4_base_index * k_bits_per_word + l4_free_bit;

        TAV_ASSERT(index < capacity());

        ++m_size;

        m_l4_map[l4_base_index] |= (1ull << l4_free_bit);
        if (m_l4_map[l4_base_index] == UINT64_MAX) {
            m_l3_map[l3_base_index] |= (1ull << l3_free_bit);
            if (m_l3_map[l3_base_index] == UINT64_MAX) {
                m_l2_map[l2_base_index] |= (1ull << l2_free_bit);
                if (m_l2_map[l2_base_index] == UINT64_MAX) {
                    m_l1_map[l1_base_index] |= (1ull << l1_free_bit);
                }
            }
        }

        return static_cast<index_t>(index);
    }

    bool l4_bitmap_index_allocator::deallocate(index_t index) noexcept
    {
        if (index >= capacity()) {
            return false;
        }

        // NOTE: Since k_bits_per_word == 64 (a power of two), the compiler
        // optimizes division and modulo into shift and mask automatically:
        //   index / 64  ->  index >> 6
        //   index % 64  ->  index & 63
        const size_t l4_bit_idx = index % k_bits_per_word;
        const size_t l4_base_index = index / k_bits_per_word;
        const uint64 l4_word = m_l4_map[l4_base_index];

        if ((l4_word & (1ull << l4_bit_idx)) == 0) {
            return false;
        }

        --m_size;
        TAV_ASSERT(m_size < capacity());

        m_l4_map[l4_base_index] &= ~(1ull << l4_bit_idx);

        // Propagate "not full" state up the hierarchy, stopping as soon as
        // a parent block was not full before this deallocation
        if (l4_word != UINT64_MAX) {
            return true;
        }

        const size_t l3_bit_idx = l4_base_index % k_bits_per_word;
        const size_t l3_base_index = l4_base_index / k_bits_per_word;
        const uint64 l3_word = m_l3_map[l3_base_index];

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

    bool l4_bitmap_index_allocator::contains(index_t index) const noexcept
    {
        if (index >= capacity()) {
            return false;
        }

        const size_t l4_bit_idx = index % k_bits_per_word;
        const size_t l4_base_index = index / k_bits_per_word;

        const uint64 l4_word = m_l4_map[l4_base_index];

        return static_cast<bool>(l4_word & (1ull << l4_bit_idx));
    }

    void l4_bitmap_index_allocator::reset() noexcept
    {
        memset(m_l1_map, 0, sizeof(m_l1_map));
        memset(m_l2_map, 0, sizeof(m_l2_map));
        memset(m_l3_map, 0, sizeof(m_l3_map));
        memset(m_l4_map, 0, sizeof(m_l4_map));
        m_size = 0;
    }

    size_t l4_bitmap_index_allocator::capacity() const noexcept
    {
        return k_capacity;
    }

    size_t l4_bitmap_index_allocator::size() const noexcept
    {
        return m_size;
    }

    index_t l4_bitmap_index_allocator::find_first() const noexcept
    {
        if (m_size == 0) {
            return static_cast<index_t>(capacity());
        }

        const uint64 l1_word = ~m_l1_map[0];
        if (l1_word == 0) {
            return static_cast<index_t>(capacity());
        }

        const size_t l2_idx = math::first_set_bit(l1_word);
        const uint64 l2_word = ~m_l2_map[l2_idx];
        if (l2_word == 0) {
            return static_cast<index_t>(capacity());
        }

        const size_t l3_idx = l2_idx * k_bits_per_word + math::first_set_bit(l2_word);
        const uint64 l3_word = ~m_l3_map[l3_idx];
        if (l3_word == 0) {
            return static_cast<index_t>(capacity());
        }

        const size_t l4_idx = l3_idx * k_bits_per_word + math::first_set_bit(l3_word);
        const uint64 l4_word = m_l4_map[l4_idx];
        if (l4_word == 0) {
            return static_cast<index_t>(capacity());
        }

        return static_cast<index_t>(l4_idx * k_bits_per_word + math::first_set_bit(l4_word));
    }

    index_t l4_bitmap_index_allocator::next_after(index_t pos) const noexcept
    {
        TAV_ASSERT(pos <= capacity());

        if (pos >= capacity()) {
            return static_cast<index_t>(capacity());
        }

        ++pos;

        while (pos < capacity()) {
            const size_t l4_idx = pos / k_bits_per_word;
            const size_t l4_bit = pos % k_bits_per_word;

            // Phase 1: check remaining bits in the current l4 word
            const uint64 l4_word = m_l4_map[l4_idx] >> l4_bit;
            if (l4_word != 0) {
                return static_cast<index_t>(l4_idx * k_bits_per_word + l4_bit + math::first_set_bit(l4_word));
            }

            // Phase 2: current l4 block exhausted - jump within the same l3 group via l3 bitmap
            const size_t next_l4 = l4_idx + 1;
            const size_t cur_l3 = l4_idx / k_bits_per_word;
            const size_t end_l4 = (cur_l3 + 1) * k_bits_per_word;

            if (next_l4 < end_l4 && next_l4 < k_l4_map_size) {
                const size_t l4_bit_in_l3 = next_l4 % k_bits_per_word;
                const uint64 l3_remaining = ~m_l3_map[cur_l3] >> l4_bit_in_l3;

                if (l3_remaining != 0) {
                    const size_t jump_l4 = next_l4 + math::first_set_bit(l3_remaining);
                    if (jump_l4 < k_l4_map_size) {
                        const uint64 jump_word = m_l4_map[jump_l4];
                        if (jump_word != 0) {
                            return static_cast<index_t>(jump_l4 * k_bits_per_word + math::first_set_bit(jump_word));
                        }
                        pos = static_cast<index_t>(jump_l4 * k_bits_per_word);
                        continue;
                    }
                }
            }

            // Phase 3: l3 group exhausted - jump within the same l2 group via l2 bitmap
            const size_t next_l3 = cur_l3 + 1;
            const size_t cur_l2 = cur_l3 / k_bits_per_word;
            const size_t end_l3 = (cur_l2 + 1) * k_bits_per_word;

            if (next_l3 < end_l3 && next_l3 < k_l3_map_size) {
                const size_t l3_bit_in_l2 = next_l3 % k_bits_per_word;
                const uint64 l2_remaining = ~m_l2_map[cur_l2] >> l3_bit_in_l2;

                if (l2_remaining != 0) {
                    const size_t jump_l3 = next_l3 + math::first_set_bit(l2_remaining);
                    if (jump_l3 < k_l3_map_size) {
                        const uint64 l3_word = ~m_l3_map[jump_l3];
                        if (l3_word != 0) {
                            const size_t jump_l4 = jump_l3 * k_bits_per_word + math::first_set_bit(l3_word);
                            if (jump_l4 < k_l4_map_size) {
                                const uint64 jump_word = m_l4_map[jump_l4];
                                if (jump_word != 0) {
                                    return static_cast<index_t>(jump_l4 * k_bits_per_word + math::first_set_bit(jump_word));
                                }
                                pos = static_cast<index_t>(jump_l4 * k_bits_per_word);
                                continue;
                            }
                        }
                        pos = static_cast<index_t>(jump_l3 * k_bits_per_word * k_bits_per_word);
                        continue;
                    }
                }
            }

            // Phase 4: l2 group exhausted - jump to the next l2 group via l1
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

            // Descend: l2 -> l3 -> l4
            const uint64 l2_word = ~m_l2_map[jump_l2];
            if (l2_word == 0) {
                pos = static_cast<index_t>((jump_l2 + 1) * k_bits_per_word * k_bits_per_word * k_bits_per_word);
                continue;
            }

            const size_t jump_l3 = jump_l2 * k_bits_per_word + math::first_set_bit(l2_word);
            if (jump_l3 >= k_l3_map_size) {
                break;
            }

            const uint64 l3_word = ~m_l3_map[jump_l3];
            if (l3_word == 0) {
                pos = static_cast<index_t>((jump_l3 + 1) * k_bits_per_word * k_bits_per_word);
                continue;
            }

            const size_t jump_l4 = jump_l3 * k_bits_per_word + math::first_set_bit(l3_word);
            if (jump_l4 >= k_l4_map_size) {
                break;
            }

            pos = static_cast<index_t>(jump_l4 * k_bits_per_word);
        }

        return static_cast<index_t>(capacity());
    }

} // namespace tavros::core

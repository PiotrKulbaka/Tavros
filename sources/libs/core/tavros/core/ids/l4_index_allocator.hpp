#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/math/bitops.hpp>

namespace tavros::core
{

    /**
     * @class l4_index_allocator
     * @brief 4-level hierarchical bit index allocator.
     * This class provides allocation and deallocation of contiguous 32-bit indices
     * using a 4-level bitmap structure for extremely fast index management.
     *
     * Features:
     * - Supports up to 64*64*64*64 = 16,777,216 indices per allocator instance.
     * - Each allocated index is unique; invalid or unallocated indices are represented
     *   by `l4_index_allocator::invalid_index` (UINT32_MAX).
     * - Allocation and deallocation are extremely fast (O(1) amortized) due to the
     *   hierarchical bitmap structure, minimizing scanning overhead.
     * - The total memory footprint of the allocator is approximately 2,097 KB (~2.1 MB),
     *   mostly consumed by the L4 map.
     *
     * Usage:
     * - Use `allocate()` to get a free index.
     * - Use `deallocate(index)` to release an index back to the pool.
     * - `reset()` clears all allocations and restores full capacity.
     *
     * @note  All indices are 0-based.
     * @note  Capacity is fixed at creation and determined by the 4-level map.
     */

    class l4_index_allocator : noncopyable
    {
    private:
        constexpr static size_t k_bits_per_word = sizeof(uint64) * 8ull;
        constexpr static size_t k_l1_map_size = 1ull;
        constexpr static size_t k_l2_map_size = 64ull;
        constexpr static size_t k_l3_map_size = 64ull * 64ull;
        constexpr static size_t k_l4_map_size = 64ull * 64ull * 64ull;
        constexpr static size_t k_total_indices = k_l4_map_size * k_bits_per_word;

    public:
        constexpr static uint32 invalid_index = UINT32_MAX;

    public:
        l4_index_allocator() noexcept = default;

        ~l4_index_allocator() noexcept = default;

        [[nodiscard]] uint32 allocate() noexcept;

        void deallocate(uint32 index) noexcept;

        void reset() noexcept;

        [[nodiscard]] size_t capacity() const noexcept;

        [[nodiscard]] size_t remaining() const noexcept;

    private:
        alignas(64) uint64 m_l1_map[k_l1_map_size] = {0};
        alignas(64) uint64 m_l2_map[k_l2_map_size] = {0};
        alignas(64) uint64 m_l3_map[k_l3_map_size] = {0};
        alignas(64) uint64 m_l4_map[k_l4_map_size] = {0};

        size_t m_remaining_indices = k_total_indices;
    };

} // namespace tavros::core

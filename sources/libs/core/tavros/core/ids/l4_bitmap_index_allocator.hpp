#pragma once

#include <tavros/core/ids/index_allocator.hpp>

namespace tavros::core
{

    /**
     * @class l4_bitmap_index_allocator
     * @brief 4-level hierarchical bitmap index allocator.
     *
     * This class provides allocation and deallocation of 32-bit indices
     * using a 4-level bitmap structure for extremely fast index management.
     *
     * Features:
     * - Supports up to 64*64*64*64 = 16,777,216 indices per allocator instance.
     * - Allocation and deallocation are O(1) amortized due to the hierarchical
     *   bitmap, minimizing scanning overhead.
     * - The total class size 2,097 KB (~2.1 MB).
     *
     * @note All indices are 0-based.
     * @note Capacity is fixed and determined by k_max_index.
     */
    class l4_bitmap_index_allocator : public index_allocator
    {
    private:
        constexpr static size_t k_bits_per_word = sizeof(uint64) * 8ull;
        constexpr static size_t k_l1_map_size = 1ull;
        constexpr static size_t k_l2_map_size = 64ull;
        constexpr static size_t k_l3_map_size = 64ull * 64ull;
        constexpr static size_t k_l4_map_size = 64ull * 64ull * 64ull;

    public:
        constexpr static size_t k_max_index = k_l4_map_size * k_bits_per_word;

    public:
        l4_bitmap_index_allocator() noexcept = default;

        ~l4_bitmap_index_allocator() noexcept override = default;

        /**
         * @brief Allocates the next free index in sequential order.
         *
         * This allocator always returns the smallest available index, growing linearly
         * until capacity is reached.
         * @see index_allocator
         */
        [[nodiscard]] index_type allocate() noexcept override;

        void deallocate(index_type index) noexcept override;

        [[nodiscard]] bool try_deallocate(index_type index) noexcept override;

        [[nodiscard]] bool allocated(index_type index) const noexcept override;

        void reset() noexcept override;

        /**
         * @brief Returns the maximum number of indices supported by this allocator.
         *
         * This function always returns the compile-time constant `k_max_index`,
         * which defines the fixed capacity of the allocator.
         *
         * @return size_t The constant maximum number of indices.
         * @note The value never changes during the lifetime of the allocator.
         * @see index_allocator
         */
        [[nodiscard]] size_t max_index() const noexcept override;

        [[nodiscard]] size_t remaining() const noexcept override;

    private:
        alignas(64) uint64 m_l1_map[k_l1_map_size] = {0};
        alignas(64) uint64 m_l2_map[k_l2_map_size] = {0};
        alignas(64) uint64 m_l3_map[k_l3_map_size] = {0};
        alignas(64) uint64 m_l4_map[k_l4_map_size] = {0};

        size_t m_remaining = k_max_index;
    };

} // namespace tavros::core

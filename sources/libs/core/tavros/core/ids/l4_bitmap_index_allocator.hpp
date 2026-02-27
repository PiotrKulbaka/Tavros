#pragma once

#include <tavros/core/ids/index_allocator.hpp>

namespace tavros::core
{

    /**
     * @class l4_bitmap_index_allocator
     * @brief 4-level hierarchical bitmap index allocator.
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
    class l4_bitmap_index_allocator final : public index_allocator
    {
    private:
        constexpr static size_t k_bits_per_word = sizeof(uint64) * 8ull;
        constexpr static size_t k_l1_map_size = 1ull;
        constexpr static size_t k_l2_map_size = 64ull;
        constexpr static size_t k_l3_map_size = 64ull * 64ull;
        constexpr static size_t k_l4_map_size = 64ull * 64ull * 64ull;

    public:
        constexpr static index_t k_max_index = static_cast<index_t>(k_l4_map_size * k_bits_per_word);

    public:
        l4_bitmap_index_allocator() noexcept;

        ~l4_bitmap_index_allocator() noexcept override = default;

        /**
         * @brief Allocates the next free index in sequential order.
         *
         * This allocator always returns the smallest available index, growing linearly
         * until capacity is reached.
         * @see index_allocator
         */
        [[nodiscard]] index_t allocate() noexcept override;

        bool deallocate(index_t index) noexcept override;

        [[nodiscard]] bool contains(index_t index) const noexcept override;

        void reset() noexcept override;

        [[nodiscard]] size_t capacity() const noexcept override;

        [[nodiscard]] size_t size() const noexcept override;

    private:
#pragma warning(push)
#pragma warning(disable : 4324)

        alignas(64) uint64 m_l1_map[k_l1_map_size];
        alignas(64) uint64 m_l2_map[k_l2_map_size];
        alignas(64) uint64 m_l3_map[k_l3_map_size];
        alignas(64) uint64 m_l4_map[k_l4_map_size];


#pragma warning(pop)

        size_t m_size;
    };

} // namespace tavros::core

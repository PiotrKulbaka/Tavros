#pragma once

#include <tavros/core/ids/index_allocator_base.hpp>

namespace tavros::core
{

    /**
     * @class l2_index_allocator
     * @brief 2-level hierarchical bit index allocator.
     * This class provides allocation and deallocation of contiguous 32-bit indices
     * using a 2-level bitmap structure for extremely fast index management.
     *
     * Features:
     * - Supports up to 64*64 = 4,096 indices per allocator instance.
     * - Allocation and deallocation are extremely fast (O(1) amortized) due to the
     *   hierarchical bitmap structure, minimizing scanning overhead.
     * - The total memory footprint of the allocator is approximately 640 Bytes.
     *
     * @note  All indices are 0-based.
     * @note  Capacity is fixed at creation and determined by the 2-level map.
     */
    class l2_index_allocator : public index_allocator_base
    {
    private:
        constexpr static size_t k_l1_map_size = 1ull;
        constexpr static size_t k_l2_map_size = 64ull;
        constexpr static size_t k_total_indices = k_l2_map_size * k_bits_per_word;

    public:
        l2_index_allocator() noexcept = default;

        ~l2_index_allocator() noexcept = default;

        /**
         * @brief Allocates and returns a free index from the allocator.
         *
         * @return index_t A valid index if available; otherwise, returns `invalid_index`.
         * @note The caller is responsible for eventually deallocating the returned index.
         * @note This function is noexcept and will not throw exceptions.
         */
        [[nodiscard]] index_t allocate() noexcept;

        /**
         * @brief Releases a previously allocated index back to the allocator.
         *
         * @param index The index to deallocate. Must have been previously returned by `allocate()`.
         * @note Behavior is undefined if the index was not allocated or has already been deallocated.
         * @note This function is noexcept and will not throw exceptions.
         */
        void deallocate(index_t index) noexcept;

        /**
         * @brief Resets the allocator, marking all indices as free.
         *
         * @note After calling this function, `remaining()` will equal `capacity()`.
         * @note All previously allocated indices become invalid.
         * @note This function is noexcept and will not throw exceptions.
         */
        void reset() noexcept;

        /**
         * @brief Returns the total capacity of the allocator.
         *
         * @return size_t The maximum number of indices that can be allocated.
         * @note This value is constant for the lifetime of the allocator instance.
         */
        [[nodiscard]] size_t capacity() const noexcept;

        /**
         * @brief Returns the number of indices currently available for allocation.
         *
         * @return size_t The number of free indices remaining.
         * @note After construction or `reset()`, this will equal `capacity()`.
         */
        [[nodiscard]] size_t remaining() const noexcept;

    private:
        alignas(64) uint64 m_l1_map[k_l1_map_size] = {0};
        alignas(64) uint64 m_l2_map[k_l2_map_size] = {0};

        size_t m_remaining_indices = k_total_indices;
    };

} // namespace tavros::core

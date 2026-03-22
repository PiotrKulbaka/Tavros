#pragma once

#include <tavros/core/ids/index_allocator.hpp>

namespace tavros::core
{

    /**
     * @brief 2-level hierarchical bitmap index allocator.
     *
     * Manages up to 4,096 indices (64 l2-words x 64 bits) using a two-level
     * bitmap structure. The l1 bitmap tracks which l2-words are completely full,
     * allowing allocation to skip over saturated blocks in O(1).
     *
     * @par Complexity
     * - `allocate()`   - O(1), uses `first_zero_bit` on l1 then l2
     * - `deallocate()` - O(1), direct bit clear
     * - `contains()`   - O(1), single bit test
     * - `find_first()` - O(64) worst case, scans l2 words
     * - `next_after()` - O(1) amortized, uses l1 to jump empty l2 blocks
     *
     * @par Memory layout
     * Both bitmaps are cache-line aligned (64 bytes) to avoid false sharing
     * and ensure efficient SIMD/prefetch behaviour.
     *
     * @note All indices are 0-based.
     * @note Capacity is fixed at compile time by @ref k_capacity.
     * @note This class is not copyable (inherits @ref index_allocator_base CRTP).
     *
     * @see index_allocator_base
     * @see index_allocator_concept
     */
    class l2_bitmap_index_allocator final : public index_allocator_base<l2_bitmap_index_allocator>
    {
        friend class index_allocator_base<l2_bitmap_index_allocator>;

    private:
        constexpr static size_t k_bits_per_word = sizeof(uint64) * 8ull;
        constexpr static size_t k_l1_map_size = 1ull;
        constexpr static size_t k_l2_map_size = 64ull;

    public:
        constexpr static index_t k_capacity = static_cast<index_t>(k_l2_map_size * k_bits_per_word);

    public:
        /**
         * @brief Constructs the allocator in a fully reset state.
         *
         * All indices are free. No dynamic allocation is performed.
         */
        l2_bitmap_index_allocator() noexcept;

        ~l2_bitmap_index_allocator() noexcept = default;

        /**
         * @brief Allocates the smallest available index.
         *
         * @return Allocated index on success, @ref invalid_index if the allocator is full.
         */
        [[nodiscard]] index_t allocate() noexcept;

        /**
         * @brief Frees a previously allocated index.
         *
         * @param index  Index to free.
         * @return       @c true if the index was allocated and is now freed,
         *               @c false if the index was out of range or not allocated.
         */
        bool deallocate(index_t index) noexcept;

        /**
         * @brief Returns @c true if @p index is currently allocated.
         * @param index  Index to test. Returns @c false for out-of-range values.
         */
        [[nodiscard]] bool contains(index_t index) const noexcept;

        /**
         * @brief Frees all allocated indices and zeroes all bitmaps.
         *
         * After reset, @ref size returns 0 and all indices are available.
         * All previously returned indices become invalid.
         */
        void reset() noexcept;

        /**
         * @brief Returns the maximum number of indices (@ref k_capacity).
         */
        [[nodiscard]] size_t capacity() const noexcept;

        /**
         * @brief Returns the number of currently allocated indices.
         */
        [[nodiscard]] size_t size() const noexcept;

    private:
        /**
         * @brief Returns the smallest allocated index, or `capacity()` if empty.
         */
        index_t find_first() const noexcept;

        /**
         * @brief Returns the next allocated index after @p pos, or `capacity()` if none.
         */
        index_t next_after(index_t pos) const noexcept;

    private:
        alignas(64) uint64 m_l1_map[k_l1_map_size];
        alignas(64) uint64 m_l2_map[k_l2_map_size];

        size_t m_size;
    };

    static_assert(index_allocator_concept<l2_bitmap_index_allocator>);

} // namespace tavros::core

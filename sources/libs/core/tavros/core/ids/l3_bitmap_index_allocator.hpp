#pragma once

#include <tavros/core/ids/index_allocator.hpp>

namespace tavros::core
{

    /**
     * @brief 3-level hierarchical bitmap index allocator.
     *
     * Manages up to 262,144 indices (64x64x64) using a three-level bitmap
     * structure. Each level tracks which blocks at the level below are completely
     * full, allowing allocation to skip saturated regions in O(1).
     *
     * @par Complexity
     * - `allocate()`   - O(1), three `first_zero_bit` calls down the hierarchy
     * - `deallocate()` - O(1), direct bit clear with up to two parent updates
     * - `contains()`   - O(1), single bit test in l3
     * - `find_first()` - O(1), three `first_set_bit` calls down the hierarchy
     * - `next_after()` - O(1) amortized, uses l2/l1 to jump empty l3/l2 blocks
     *
     * @par Memory layout
     * All three bitmaps are cache-line aligned (64 bytes).
     * Total size: 33,408 bytes (~32.7 KB).
     *
     * @note All indices are 0-based.
     * @note Capacity is fixed at compile time by @ref k_capacity.
     *
     * @see index_allocator_base
     * @see index_allocator_concept
     */
    class l3_bitmap_index_allocator final : public index_allocator_base<l3_bitmap_index_allocator>
    {
        friend class index_allocator_base<l3_bitmap_index_allocator>;

    private:
        constexpr static size_t k_bits_per_word = sizeof(uint64) * 8ull;
        constexpr static size_t k_l1_map_size = 1ull;
        constexpr static size_t k_l2_map_size = 64ull;
        constexpr static size_t k_l3_map_size = 64ull * 64ull;

    public:
        constexpr static index_t k_capacity = static_cast<index_t>(k_l3_map_size * k_bits_per_word);

    public:
        /**
         * @brief Constructs the allocator in a fully reset state.
         *
         * All indices are free. No dynamic allocation is performed.
         */
        l3_bitmap_index_allocator() noexcept;

        ~l3_bitmap_index_allocator() noexcept = default;

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
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier

        alignas(64) uint64 m_l1_map[k_l1_map_size];
        alignas(64) uint64 m_l2_map[k_l2_map_size];
        alignas(64) uint64 m_l3_map[k_l3_map_size];

#pragma warning(pop)

        size_t m_size;
    };

    static_assert(index_allocator_concept<l3_bitmap_index_allocator>);

} // namespace tavros::core

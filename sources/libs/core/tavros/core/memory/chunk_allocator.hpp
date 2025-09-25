#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/noncopyable.hpp>

namespace tavros::core
{

    /**
     * @class chunk_allocator
     * @brief Fixed-size block allocator optimized for cache efficiency.
     *
     * This allocator manages a fixed-size memory pool divided into blocks of type T.
     * It does not own memory itself, but operates on a memory buffer provided at construction.
     *
     * Features and design choices:
     * - Maximum of 512 blocks by default; chosen to match two CPU cache lines:
     *   - Each cache line is 64 bytes.
     *   - The internal bitmap uses 64-bit words, allowing each word to represent 64 blocks.
     *   - This ensures that the bitmap fits within two cache lines (128 bytes), providing
     *     optimal cache locality for allocation and deallocation operations.
     * - Bitmask-based free block tracking:
     *   - Each bit in the bitmap represents the allocation state of a block.
     *   - Allows fast O(1) allocation in most cases.
     * - Safe deallocation:
     *   - Checks that the pointer belongs to the allocator and is correctly aligned.
     *   - If invalid pointers are passed, the method returns safely without corrupting state.
     * - Lightweight and non-owning:
     *   - The allocator only manages memory handed to it.
     *   - For larger pools or dynamic memory management, use a pool_allocator that aggregates
     *     multiple chunk_allocators.
     * - Hot-path optimized:
     *   - Allocation loops are short (maximum 8 iterations for 512 blocks).
     *   - Memory and bitmap are aligned to 64-byte boundaries for optimal cache line usage.
     * Notes:
     * - The class is non-copyable.
     * - It is designed for high-performance, deterministic allocation patterns,
     *   suitable for real-time engines and long-running applications.
     */

    template<class T, size_t MaxBlocksNumber = 512 /* 8 * 64 */>
    class chunk_allocator : noncopyable
    {
    public:
        constexpr static size_t k_type_align = (alignof(T) + 7) & ~7;
        constexpr static size_t k_type_size = (sizeof(T) + 7) & ~7;
        constexpr static size_t k_map_size = MaxBlocksNumber / 64;

        static_assert(MaxBlocksNumber % 512 == 0, "MaxBlocksNumber must be a multiple of 512");
        static_assert(k_type_align % 8 == 0, "k_type_align must be a multiple of 8");
        static_assert(k_type_size % 8 == 0, "k_type_size must be a multiple of 8");

    public:
        /**
         * @brief Constructs a chunk_allocator using a pre-allocated memory buffer.
         *
         * Initializes the allocator to manage a contiguous memory region divided into fixed-size blocks.
         * All blocks are initially free.
         *
         * @param storage Pointer to a pre-allocated memory buffer. Must be aligned to `k_type_align`.
         * @param total_blocks Total number of blocks in the memory buffer. Must be > 0 and <= MaxBlocksNumber.
         *
         * @pre `storage` must not be nullptr.
         * @pre `storage` must be aligned to `k_type_align`.
         * @pre `total_blocks` must be greater than zero.
         */
        chunk_allocator(uint8* storage, size_t total_blocks)
            : m_storage(storage)
            , m_total_blocks(total_blocks)
            , m_available_blocks(total_blocks)
        {
            TAV_ASSERT(m_storage != nullptr);
            TAV_ASSERT(m_total_blocks > 0);
            TAV_ASSERT(reinterpret_cast<size_t>(m_storage) % k_type_align == 0);

            for (size_t i = 0; i < k_map_size; ++i) {
                m_map[i] = 0;
            }

            m_end = m_storage + m_total_blocks * k_type_size;
            m_last_block = m_total_blocks / 64;
            m_last_bit = m_total_blocks % 64;
        }

        ~chunk_allocator() = default;

        /**
         * @brief Checks whether a pointer belongs to this allocator.
         *
         * @param ptr Pointer to check.
         * @return true if the pointer lies within the memory range managed by this allocator; false otherwise.
         *
         * @note The pointer does not need to be currently allocated; it only checks ownership of memory.
         */
        bool owns(T* ptr) const noexcept
        {
            auto* p = reinterpret_cast<uint8*>(ptr);
            return p >= m_storage && p < m_end;
        }

        /**
         * @brief Returns the number of blocks that are currently free.
         *
         * @return Number of available (unallocated) blocks.
         */
        [[nodiscard]] size_t available_blocks() const noexcept
        {
            return m_available_blocks;
        }

        /**
         * @brief Checks if the allocator has no free blocks.
         *
         * @return true if all blocks are allocated; false otherwise.
         */
        [[nodiscard]] bool filled() const noexcept
        {
            return m_available_blocks == 0;
        }

        /**
         * @brief Allocates a single block from the allocator.
         *
         * @return Pointer to a free block of type T, or nullptr if no blocks are available.
         */
        [[nodiscard]] T* allocate_block()
        {
            for (size_t wi = 0; wi < m_last_block; ++wi) {
                uint64 word = m_map[wi];
                if (word != std::numeric_limits<uint64>::max()) {
                    uint32 free_bit_index = first_zero_bit(word);
                    m_map[wi] |= (1ull << free_bit_index);
                    TAV_ASSERT(m_available_blocks > 0);
                    m_available_blocks--;
                    return reinterpret_cast<T*>(m_storage + k_type_size * (wi * 64 + free_bit_index));
                }
            }
            if (m_last_bit != 0) {
                uint64 word = m_map[m_last_block];
                uint32 free_bit_index = first_zero_bit(word);
                if (free_bit_index < m_last_bit) {
                    m_map[m_last_block] |= (1ull << free_bit_index);
                    TAV_ASSERT(m_available_blocks > 0);
                    m_available_blocks--;
                    return reinterpret_cast<T*>(m_storage + k_type_size * (m_last_block * 64 + free_bit_index));
                }
            }
            return nullptr;
        }

        /**
         * @brief Deallocates a previously allocated block.
         *
         * @param ptr Pointer to the block to release.
         */
        void deallocate_block(T* ptr)
        {
            uint8* p = reinterpret_cast<uint8*>(ptr);

            TAV_ASSERT(p >= m_storage);
            if (p < m_storage) {
                return;
            }

            TAV_ASSERT(p < m_end);
            if (p >= m_end) {
                return;
            }

            size_t offset = p - m_storage;
            TAV_ASSERT(offset % k_type_size == 0);
            if (offset % k_type_size != 0) {
                return;
            }

            size_t index = offset / k_type_size;
            size_t word_index = index / 64;
            size_t bit_index = index % 64;
            uint64 bit_mask = (1ull << bit_index);
            uint64 word = m_map[word_index];

            TAV_ASSERT(word & bit_mask);

            if (word & bit_mask) {
                m_map[word_index] &= ~bit_mask;
                m_available_blocks++;
            }
        }

        static uint32 first_zero_bit(uint64 x)
        {
#if defined(_MSC_VER)
            unsigned long idx;
            if (_BitScanForward64(&idx, ~x)) {
                return idx;
            }
            return 64;
#else
            return __builtin_ffsll(~x) - 1; // GCC/Clang
#endif
        }

    private:
        alignas(64) uint64 m_map[k_map_size] = {0};
        uint8* m_storage = nullptr;
        uint8* m_end = nullptr;
        size_t m_total_blocks = 0;
        size_t m_available_blocks = 0;
        size_t m_last_block = 0;
        size_t m_last_bit = 0;
    };


} // namespace tavros::core

#pragma once

#include <tavros/core/memory/allocator.hpp>
#include <tavros/core/types.hpp>
#include <tavros/core/debug/assert.hpp>

namespace tavros::core
{

    /**
     * Memory Allocation Terminology
     *
     * Defines the hierarchy of memory units used in custom allocators:
     * Pool > Chunk > Block.
     *
     * Block:
     *   - The smallest allocatable unit of memory.
     *   - Each block is aligned to alignof(T), with a minimal enforced alignment of 8 bytes.
     *   - Minimal block size is 8 bytes.
     *   - Blocks are directly handed out to the user upon allocation.
     *
     * Chunk:
     *   - A larger contiguous memory region consisting of multiple blocks.
     *   - Chunk size is implementation-defined, often a multiple of a memory page.
     *   - Serves as the container that subdivides memory into blocks.
     *
     * Pool:
     *   - A top-level container that manages multiple chunks.
     *   - Grows dynamically by acquiring new chunks from the system allocator.
     *   - Manages recycling of freed blocks to reduce system allocation overhead.
     *
     * Hierarchy:
     *   Pool -> Chunk -> Block
     *
     * Example:
     *   - A pool requests a 64 KB chunk from the system allocator.
     *   - The chunk is divided into 8192 blocks of 8 bytes each.
     *   - Allocations are satisfied by providing one or more blocks.
     */

    template<typename T>
    class pool_allocator
    {
    public:
        static constexpr size_t TypeAlign = (alignof(T) + 7) & ~7;
        static constexpr size_t TypeSize = (sizeof(T) + 7) & ~7;

    public:
        pool_allocator(allocator* allocator, size_t initial_elements_count = 0)
            : m_allocator(allocator)
        {
            TAV_ASSERT(allocator != nullptr);

            m_max_chunk_count = 128;
            m_last_block_allocation_size = initial_elements_count < 64 ? 64 : initial_elements_count;
            m_chunks = allocator->allocate<chunk>(m_max_chunk_count, "pool allocator chunks");
        }

        T* allocate()
        {
            for (size_t i = 0; i < m_chunk_count; ++i) {
                if (m_chunks[i].has_free_block()) {
                    return m_chunks[i].allocate_block();
                }
            }

            if (m_chunk_count < m_max_chunk_count) {
                m_last_block_allocation_size *= 2;

                if (m_last_block_allocation_size > chunk::k_blocks_per_chunk) {
                    m_last_block_allocation_size = chunk::k_blocks_per_chunk;
                }

                m_chunks[m_chunk_count].init(m_allocator->allocate<uint8>(m_last_block_allocation_size * TypeSize, "pool allocator chunk"), m_last_block_allocation_size);
                m_chunk_count++;
                return m_chunks[m_chunk_count - 1].allocate_block();
            }

            TAV_ASSERT(false);

            return nullptr;
        }

        void deallocate(T* ptr)
        {
        }

    private:
        struct chunk
        {
            constexpr static size_t k_blocks_per_chunk = 512; // 8 * 64
            constexpr static size_t k_map_size = k_blocks_per_chunk / 64;

            static_assert(k_blocks_per_chunk % 512 == 0, "k_blocks_per_chunk must be a multiple of 512");
            static_assert(k_map_size % 8 == 0, "k_map_size must be a multiple of 8");


            alignas(64) uint64 bitmap[k_map_size] = {0};
            uint8* storage = nullptr;
            size_t total_blocks = 0;
            size_t available_blocks = 0;

            void init(uint8* storage, size_t total_blocks)
            {
                TAV_ASSERT(storage != nullptr);
                // total_blocks must be a multiple of 64, because each block is aligned to 64 bits
                TAV_ASSERT(total_blocks % 64 == 0);

                this->storage = storage;
                this->total_blocks = total_blocks;
                this->available_blocks = total_blocks;
                for (size_t i = 0; i < k_map_size; ++i) {
                    bitmap[i] = 0;
                }
            }

            bool owns(T* ptr) const
            {
                return ptr >= storage && ptr < storage + total_blocks * TypeSize;
            }

            bool has_free_block() const
            {
                return available_blocks > 0;
            }

            T* allocate_block()
            {
                size_t max_size = total_blocks / 64;
                for (size_t wi = 0; wi < max_size; ++wi) {
                    uint64 word = bitmap[wi];
                    if (word != std::numeric_limits<uint64>::max()) {
                        uint64 free_bit = ~word & (word + 1);
                        TAV_ASSERT(free_bit != 0);
                        uint32 free_bit_index = first_zero_bit(first_free_bit);
                        bitmap[wi] |= (1ull << free_bit_index);
                        available_blocks--;
                        return reinterpret_cast<T*>(storage + TypeSize * (wi * 64 + free_bit_index));
                    }
                }
                TAV_ASSERT(false);
                return nullptr;
            }

            void deallocate_block(T* ptr)
            {
                TAV_ASSERT(ptr >= storage);
                TAV_ASSERT(ptr < storage + total_blocks * TypeSize);
                TAV_ASSERT(reinterpret_cast<size_t>(ptr) % TypeAlign == 0);

                size_t index = static_cast<size_t>(ptr - storage) / TypeSize;
                size_t word_index = index / 64;
                size_t bit_index = index % 64;
                TAV_ASSERT((bitmap[word_index] & (1ull << bit_index)) != 0);
                bitmap[word_index] &= ~(1ull << bit_index); // освобождаем бит
                available_blocks++;
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
        }; // struct chunk

    private:
        allocator* m_allocator;
        chunk*     m_chunks = nullptr;
        size_t     m_chunk_count = 0;
        size_t     m_max_chunk_count = 0;
        size_t     m_last_block_allocation_size = 0;
    };

} // namespace tavros::core
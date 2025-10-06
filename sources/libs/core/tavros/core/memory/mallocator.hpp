#pragma once

#include <tavros/core/memory/allocator.hpp>
#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/defines.hpp>

namespace tavros::core
{

    /**
     * @brief A memory allocator implementation based on aligned heap allocation.
     *
     * This allocator provides aligned dynamic memory management using
     * platform-specific functions (`aligned_malloc`, `aligned_realloc`, and `aligned_free`).
     * It ensures that allocated memory respects the specified alignment, which must be a power of two.
     *
     * The allocator also includes optional debug checks that verify allocation
     * and deallocation correctness, helping detect memory leaks or invalid frees
     * during development.
     *
     * This class serves as a general-purpose aligned allocator, suitable for
     * systems that require explicit memory alignment (e.g., SIMD data, GPU resources, etc.).
     */
    class mallocator final : public allocator
    {
    public:
        mallocator();

        ~mallocator() override;

        void* allocate(size_t size, size_t align, const char* tag = nullptr) override;

        void* reallocate(void* ptr, size_t new_size, size_t align, const char* tag = nullptr) override;

        void deallocate(void* ptr) override;

        void clear() override;

    private:
        // #if TAV_DEBUG
        struct allocation_info
        {
            size_t      size;
            size_t      align;
            const char* tag;
        };

        unordered_map<void*, allocation_info> m_allocations;
        unordered_map<void*, allocation_info> m_released;
        // #endif // TAV_DEBUG
    };

} // namespace tavros::core

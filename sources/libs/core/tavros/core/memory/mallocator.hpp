#pragma once

#include <tavros/core/memory/allocator.hpp>
#include <tavros/core/pimpl.hpp>
#include <tavros/core/defines.hpp>

namespace tavros::core
{

    /**
     * @brief A memory allocator implementation using malloc and free.
     *
     * This class provides a simple allocator that delegates memory allocation
     * and deallocation to the standard C library functions `malloc` and `free`.
     * It is suitable for general-purpose use, testing, or as a baseline allocator.
     *
     * Internally, it uses a PIMPL (Pointer to IMPLementation) pattern to hide implementation details.
     */
    class mallocator final : public allocator
    {
    public:
        /**
         * @brief Constructs a new mallocator instance.
         *
         * Initializes internal state required for allocation and deallocation.
         */
        mallocator();

        /**
         * @brief Destroys the mallocator.
         *
         * Cleans up any internal state. All allocated memory should be freed before destruction.
         */
        virtual ~mallocator();

        /**
         * @brief Allocates a block of memory of the given size.
         *
         * @param size The number of bytes to allocate.
         * @param tag Optional string tag to identify or categorize the allocation (ignored in this implementation).
         * @return Pointer to the allocated memory block, or nullptr if allocation fails.
         */
        void* allocate(size_t size, size_t align, const char* tag = nullptr) override;

        /**
         * @brief Frees a previously allocated block of memory.
         * @note Passing a pointer that was not allocated by this allocator,
         *       or that has already been deallocated, results in undefined behavior.
         *
         * @param ptr Pointer to the memory block to deallocate. Must be a pointer previously returned by allocate().
         */
        void deallocate(void* ptr) override;

        /**
         * @brief Clears all internal state.
         *
         * This implementation does not manage pooled memory, so this method performs no operation.
         * It is provided to satisfy the allocator interface.
         */
        void clear() override;

    private:
        struct impl;
        static constexpr size_t              impl_size = TAV_DEBUG ? pointer_size * 20 : pointer_size * 16;
        pimpl<impl, impl_size, pointer_size> m_impl;
    };

} // namespace tavros::core

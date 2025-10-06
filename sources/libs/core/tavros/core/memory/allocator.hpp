#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/noncopyable.hpp>

namespace tavros::core
{

    /**
     * @brief Interface for memory allocation strategies.
     *
     * This class defines a generic interface for memory allocators.
     * It allows for custom memory management implementations that support
     * allocation, deallocation, and clearing of memory.
     *
     * Custom allocators can be used for performance tuning, debugging,
     * memory tracking, or platform-specific optimization.
     */
    class allocator : noncopyable
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures derived classes can be properly destroyed through a base class pointer.
         */
        virtual ~allocator() = default;

        /**
         * @brief Allocates a block of memory.
         *
         * @param size The number of bytes to allocate.
         * @param align Memory alignment, should be opw of two.
         * @param tag Optional string tag to identify or categorize the allocation (for debugging or profiling).
         * @return Pointer to the allocated memory block, or nullptr if allocation fails.
         */
        virtual void* allocate(size_t size, size_t align, const char* tag = nullptr) = 0;

        /**
         * @brief Reallocates a previously allocated memory block to a new size.
         *
         * This function attempts to resize an existing memory block referenced by @p ptr
         * to @p new_size bytes while preserving its contents up to the minimum of the
         * old and new sizes. If the existing block cannot be resized in place, a new
         * block is allocated, the data is copied, and the old block is freed.
         *
         * @param ptr Pointer to the previously allocated memory block. Can be nullptr,
         *        in which case this function behaves like @c allocate().
         * @param new_size The new size of the memory block, in bytes.
         * @param align Memory alignment, must be the same as the alignment used during
         *        the original allocation. Typically a power of two.
         * @param tag Optional string tag used for debugging, profiling, or tracking allocations.
         * @return Pointer to the reallocated memory block, or nullptr if allocation fails.
         *         If nullptr is returned, the original memory remains valid and must be freed
         *         by the caller if no longer needed.
         */
        virtual void* reallocate(void* ptr, size_t new_size, size_t align, const char* tag = nullptr) = 0;

        /**
         * @brief Frees a previously allocated block of memory.
         * @note Passing a pointer that was not allocated by this allocator,
         *       or that has already been deallocated, results in undefined behavior.
         *
         * @param ptr Pointer to the memory block to deallocate. Must be a pointer previously returned by allocate().
         */
        virtual void deallocate(void* ptr) = 0;

        /**
         * @brief Clears all allocations, if supported.
         *
         * This method allows the allocator to release all held memory in one operation.
         * Not all allocator implementations may support this operation.
         */
        virtual void clear() = 0;
    };

} // namespace tavros::core

#pragma once

#include <tavros/core/ids/index.hpp>
#include <tavros/core/noncopyable.hpp>

namespace tavros::core
{

    /**
     * @class index_allocator
     * @brief Base interface for all index allocators.
     *
     * This class defines the fundamental operations for index allocation
     * and management. It does not provide an implementation, only the API
     * contract to be followed by derived classes.
     *
     * @note All indices are 0-based.
     */
    class index_allocator
    {
    public:
        /**
         * @brief Virtual destructor for safe polymorphic destruction.
         */
        virtual ~index_allocator() noexcept = default;

        /**
         * @brief Allocates and returns a free index from the allocator.
         *
         * @return index_type A valid index if available; otherwise, returns `invalid_index`.
         * @note The caller is responsible for eventually deallocating the returned index.
         */
        virtual [[nodiscard]] index_type allocate() noexcept = 0;

        /**
         * @brief Releases a previously allocated index back to the allocator.
         *
         * @param index The index to deallocate. Must have been previously returned by `allocate()`.
         * @note Behavior is undefined if the index was not allocated or has already been deallocated.
         */
        virtual void deallocate(index_type index) noexcept = 0;

        /**
         * @brief Attempts to deallocate a previously allocated index.
         *
         * @param index The index to deallocate. Must have been previously returned by `allocate()`.
         * @return `true` if the index was successfully deallocated, or `false` if the index
         *         was not currently allocated.
         */
        virtual [[nodiscard]] bool try_deallocate(index_type index) noexcept = 0;

        /**
         * @brief Checks whether the given index is currently allocated.
         *
         * @param index The index to check.
         * @return `true` if the index is currently allocated, or `false` otherwise.
         */
        virtual [[nodiscard]] bool allocated(index_type index) const noexcept = 0;

        /**
         * @brief Resets the allocator, marking all indices as free.
         *
         * @note After calling this function, `remaining()` will equal `max_index()`.
         * @note All previously allocated indices become invalid.
         */
        virtual void reset() noexcept = 0;

        /**
         * @brief Returns the total capacity of the allocator.
         *
         * @return size_t The maximum number of indices that can ever be allocated.
         * @note This value is constant for the lifetime of the allocator instance.
         */
        virtual [[nodiscard]] size_t max_index() const noexcept = 0;

        /**
         * @brief Returns the number of indices currently available for allocation.
         *
         * @return size_t The number of free indices remaining.
         * @note After construction or `reset()`, this will equal `max_index()`.
         */
        virtual [[nodiscard]] size_t remaining() const noexcept = 0;
    };

} // namespace tavros::core

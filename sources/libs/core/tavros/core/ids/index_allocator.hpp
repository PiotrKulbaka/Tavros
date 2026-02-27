#pragma once

#include <tavros/core/ids/index_base.hpp>
#include <tavros/core/noncopyable.hpp>

namespace tavros::core
{

    /**
     * @class index_allocator
     * @brief Abstract base for index allocators.
     *
     * Manages a set of integer indices in range [0, capacity()).
     * All operations are noexcept - errors reported via return values.
     *
     * @note All indices are 0-based.
     */
    class index_allocator
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~index_allocator() noexcept = default;

        /**
         * @brief Allocate the next free index.
         *
         * @return Valid index, or invalid_index if capacity exhausted.
         */
        [[nodiscard]] virtual index_t allocate() noexcept = 0;

        /**
         * @brief Deallocate a previously allocated index.
         *
         * @param index the index to deallocate.
         * @return true if deallocated, false if index was not allocated.
         * @note Safe to call with invalid or already-freed index.
         */
        virtual bool deallocate(index_t index) noexcept = 0;

        /**
         * @brief Check if index is currently allocated.
         *
         * @param index The index to check.
         * @return `true` if the index is currently allocated, or `false` otherwise.
         */
        [[nodiscard]] virtual bool contains(index_t index) const noexcept = 0;

        /**
         * @brief Reset all indices to free state.
         */
        virtual void reset() noexcept = 0;

        /**
         * @brief Returns the maximum number of indices.
         */
        [[nodiscard]] virtual size_t capacity() const noexcept = 0;

        /**
         * @brief Returns the number of currently allocated indices.
         */
        [[nodiscard]] virtual size_t size() const noexcept = 0;

        /**
         * @brief Returns 'true' if no index is allocated.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return size() == 0;
        }

        /**
         * @brief Returns 'true' if the allocator has allocated all possible indices.
         */
        [[nodiscard]] bool full() const noexcept
        {
            return size() == capacity();
        }

        /**
         * @brief Returns the number of free indexes.
         */
        [[nodiscard]] size_t available() const noexcept
        {
            return capacity() - size();
        }
    };

} // namespace tavros::core

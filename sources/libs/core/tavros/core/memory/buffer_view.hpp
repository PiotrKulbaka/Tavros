#pragma once

#include <tavros/core/memory/dynamic_buffer.hpp>

namespace tavros::core
{

    /**
     * @brief A lightweight, non-owning read-only view over a contiguous memory block.
     *
     * This class provides a safe way to access a sequence of elements without owning them.
     * Similar to std::string_view, it does not manage memory lifetime.
     *
     * @tparam T Element type.
     */
    template<class T>
    class buffer_view
    {
    public:
        /**
         * @brief Constructs an empty view.
         */
        constexpr buffer_view() noexcept = default;

        /**
         * @brief Constructs a view from a pointer and number of elements.
         * @param data Pointer to the first element (can be nullptr if size is zero).
         * @param size Number of elements in the buffer.
         */
        constexpr buffer_view(const T* data, size_t size) noexcept
            : m_data(data)
            , m_size(size)
        {
        }

        /**
         * @brief Constructs a view over the given dynamic_buffer.
         *
         * Initializes the view to reference the underlying data and capacity
         * of the specified dynamic_buffer without taking ownership.
         *
         * @param buffer Reference to the source dynamic_buffer.
         */
        constexpr buffer_view(const dynamic_buffer<T>& buffer) noexcept
            : m_data(buffer.data())
            , m_size(buffer.capacity())
        {
        }

        /**
         * @brief Returns the number of elements in the view.
         */
        [[nodiscard]] constexpr size_t size() const noexcept
        {
            return m_size;
        }

        /**
         * @brief Returns whether the view is empty.
         */
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return m_size == 0;
        }

        /**
         * @brief Returns a pointer to the underlying memory block.
         */
        [[nodiscard]] constexpr const T* data() const noexcept
        {
            return m_data;
        }

        /**
         * @brief Accesses an element by index.
         * @note Behavior is undefined if index >= size().
         */
        [[nodiscard]] constexpr const T& operator[](size_t index) const noexcept
        {
            TAV_ASSERT(index < m_size);
            return m_data[index];
        }

        [[nodiscard]] constexpr const T* begin() const noexcept
        {
            return m_data;
        }

        [[nodiscard]] constexpr const T* end() const noexcept
        {
            return m_data + m_size;
        }

    private:
        const T* m_data = nullptr;
        size_t   m_size = 0;
    };

} // namespace tavros::core

#pragma once

#include <tavros/core/memory/dynamic_buffer.hpp>

namespace tavros::core
{

    /**
     * @brief Non-owning mutable reference to a contiguous memory region.
     *
     * Similar to std::span, this class represents a view over an existing
     * memory buffer, allowing both read and write access to its contents.
     *
     * The class does not manage memory — it only stores a pointer and a size.
     *
     * @tparam T Element type of the buffer.
     */
    template<typename T>
    class buffer_span
    {
    public:
        /**
         * @brief Constructs an empty span.
         */
        constexpr buffer_span() noexcept = default;

        /**
         * @brief Constructs a span from a pointer and a size.
         *
         * @param data Pointer to the first element.
         * @param size Number of elements in the buffer.
         */
        constexpr buffer_span(T* data, size_t size) noexcept
            : m_data(data)
            , m_size(size)
        {
        }

        /**
         * @brief Constructs a span from a dynamic_buffer.
         *
         * The span references the buffer’s data directly and allows modification.
         * Ownership is not transferred.
         *
         * @param buffer Source dynamic_buffer.
         */
        constexpr buffer_span(dynamic_buffer<T>& buffer) noexcept
            : m_data(buffer.data())
            , m_size(buffer.capacity())
        {
        }

        /**
         * @brief Returns the number of elements in the buffer.
         */
        [[nodiscard]] constexpr size_t size() const noexcept
        {
            return m_size;
        }

        /**
         * @brief Returns whether the span is empty.
         */
        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return m_size == 0;
        }

        /**
         * @brief Returns a pointer to the beginning of the buffer.
         */
        [[nodiscard]] constexpr T* data() noexcept
        {
            return m_data;
        }

        /**
         * @brief Returns a const pointer to the beginning of the buffer.
         */
        [[nodiscard]] constexpr const T* data() const noexcept
        {
            return m_data;
        }

        /**
         * @brief Accesses an element by index.
         * @note Behavior is undefined if index >= size().
         */
        [[nodiscard]] constexpr T& operator[](size_t index) noexcept
        {
            TAV_ASSERT(index < m_size);
            return m_data[index];
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

        [[nodiscard]] constexpr T* begin() noexcept
        {
            return m_data;
        }

        [[nodiscard]] constexpr const T* begin() const noexcept
        {
            return m_data;
        }

        [[nodiscard]] constexpr T* end() noexcept
        {
            return m_data + m_size;
        }

        [[nodiscard]] constexpr const T* end() const noexcept
        {
            return m_data + m_size;
        }

    private:
        T*     m_data = nullptr;
        size_t m_size = 0;
    };

} // namespace tavros::core

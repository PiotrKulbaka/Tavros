#pragma once

#include <tavros/core/memory/dynamic_buffer.hpp>
#include <concepts>

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
         * @brief Constructs an empty span from nullptr.
         *
         * Useful to represent an empty span.
         */
        constexpr buffer_span(std::nullptr_t) noexcept
            : m_data(nullptr)
            , m_size(0)
        {
        }

        /**
         * @brief Constructs a span over a single object.
         * @param single_obj Reference to a single object.
         */
        constexpr buffer_span(T& single_obj) noexcept
            : m_data(&single_obj)
            , m_size(1)
        {
        }

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
         * @brief Constructs a span from a fixed-size array.
         *
         * @tparam N Size of the array (deduces the size automatically).
         * @param arr Reference to the array.
         */
        template<size_t N>
        constexpr buffer_span(T (&arr)[N]) noexcept
            : m_data(arr)
            , m_size(N)
        {
        }

        template<class Container>
            requires requires(Container c) { c.data(); c.size(); }
        constexpr buffer_span(Container& c) noexcept
            : m_data(c.data())
            , m_size(c.size())
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
         * @brief Copies elements from the source array into the buffer span.
         *
         * @param src Pointer to the source data.
         * @param count Number of elements to copy.
         * @param offset Starting offset in the destination buffer (in elements).
         * @return The number of elements actually copied.
         */
        size_t copy_from(const void* src, size_t count, size_t offset = 0)
        {
            TAV_ASSERT(offset + count <= m_size);
            std::memcpy(m_data + offset, src, count * sizeof(T));
            return count;
        }

        /**
         * @brief Fills a portion of the buffer with a specified value.
         *
         * Writes @p value into the range [offset, offset + count).
         * If the range exceeds the buffer capacity, the operation is trimmed.
         *
         * @param offset Starting index.
         * @param count Number of elements to fill.
         * @param value The value to assign to each element.
         */
        void fill(size_t offset, size_t count, const T& value) noexcept
        {
            TAV_ASSERT(offset + count <= m_size);

            std::fill_n(m_data + offset, count, value);
        }

        /**
         * @brief Fills the entire buffer with the specified value.
         *
         * @param value The value to assign to each element.
         */
        void fill(const T& value) noexcept
        {
            std::fill_n(m_data, m_size, value);
        }

        /**
         * @brief Creates a view of the buffer starting at the specified offset.
         *
         * Returns a non-owning span over the range [offset, size).
         * The caller must ensure that @p offset is within the buffer bounds.
         *
         * @param offset Starting index of the slice.
         *
         * @return A span covering elements from @p offset to the end of the buffer.
         */
        [[nodiscard]] buffer_span<T> slice(size_t offset) const noexcept
        {
            TAV_ASSERT(offset <= m_size);
            return {m_data + offset, m_size - offset};
        }

        /**
         * @brief Creates a view of a subrange of the buffer.
         *
         * Returns a non-owning span over the range [offset, offset + count).
         * The caller must ensure that the specified range is fully contained
         * within the buffer bounds.
         *
         * @param offset Starting index of the slice.
         * @param count Number of elements in the slice.
         *
         * @return A span covering the specified subrange of the buffer.
         */
        [[nodiscard]] buffer_span<T> slice(size_t offset, size_t count) const noexcept
        {
            TAV_ASSERT(offset + count <= m_size);
            return {m_data + offset, count};
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

        /**
         * @brief Allows checking for empty.
         */
        explicit operator bool() const noexcept
        {
            return m_size != 0;
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

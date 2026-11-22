#pragma once

#include <tavros/core/memory/dynamic_buffer.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace tavros::core
{

    /**
     * @brief Non-owning view over a contiguous memory region.
     *
     * Similar to std::span, this class represents a view over an existing
     * memory buffer. Whether the view is read-only or read-write depends
     * solely on the constness of @p T:
     *   - buffer_view<T>       - mutable view (read/write access)
     *   - buffer_view<const T> - read-only view
     *
     * Constness is reflected in the member type aliases (element_type, pointer,
     * reference, iterator), and modifying functions (copy_from, fill) are only
     * available when @p T is non-const.
     *
     * The class does not manage memory - it only stores a pointer and a size.
     *
     * @tparam T Element type of the buffer (may be const).
     */
    template<class T>
    class buffer_view
    {
        static_assert(!std::is_reference_v<T>, "buffer_view<T>: T must not be a reference");
        static_assert(!std::is_void_v<T>, "buffer_view<T>: T must not be void");

    public:
        /// Element type including constness (const U for read-only views).
        using element_type = T;
        /// Element type without cv-qualifiers.
        using value_type = std::remove_cv_t<T>;
        using size_type = size_t;
        using pointer = T*;
        using const_pointer = const value_type*;
        using reference = T&;
        using const_reference = const value_type&;
        using iterator = pointer;
        using const_iterator = const_pointer;

        /// True if the view is read-only.
        static constexpr bool is_read_only = std::is_const_v<T>;

        /**
         * @brief Constructs an empty view.
         */
        constexpr buffer_view() noexcept = default;

        /**
         * @brief Constructs an empty view from nullptr.
         */
        constexpr buffer_view(std::nullptr_t) noexcept
            : m_data(nullptr)
            , m_size(0)
        {
        }

        /**
         * @brief Constructs a view over a single object.
         * @param single_obj Reference to a single object.
         */
        constexpr buffer_view(T& single_obj) noexcept
            : m_data(&single_obj)
            , m_size(1)
        {
        }

        /**
         * @brief Constructs a view from an initializer list.
         * @param list Initializer list of elements.
         */
        constexpr buffer_view(std::initializer_list<std::remove_const_t<T>> list) noexcept
            : m_data(list.begin())
            , m_size(static_cast<size_type>(list.size()))
        {
        }

        /**
         * @brief Constructs a view from a pointer and a size.
         *
         * @param data Pointer to the first element (may be nullptr if size is zero).
         * @param size Number of elements in the buffer.
         */
        constexpr buffer_view(T* data, size_t size) noexcept
            : m_data(data)
            , m_size(size)
        {
        }

        /**
         * @brief Constructs a view from a fixed-size array.
         *
         * @tparam N Size of the array (deduced automatically).
         * @param arr Reference to the array.
         */
        template<size_t N>
        constexpr buffer_view(T (&arr)[N]) noexcept
            : m_data(arr)
            , m_size(N)
        {
        }

        /**
         * @brief Constructs a view from any container that provides data() and size().
         *
         * For buffer_view<T> (mutable) this only participates in overload
         * resolution if @p c.data() yields a pointer convertible to T*,
         * i.e. the container itself must be non-const.
         * For buffer_view<const T> it accepts both const and non-const containers.
         */
        template<class Container>
            requires(!std::is_same_v<std::remove_cvref_t<Container>, buffer_view>)
                     && requires(Container& c) {
                            { c.data() } -> std::convertible_to<T*>;
                            { c.size() } -> std::convertible_to<size_t>;
                        }
        constexpr buffer_view(Container& c) noexcept
            : m_data(c.data())
            , m_size(c.size())
        {
        }

        /**
         * @brief Constructs a mutable view over a dynamic_buffer.
         *
         * Enabled only for buffer_view<T> with non-const T.
         */
        template<class U>
            requires(!std::is_const_v<T> && std::is_same_v<T, U>)
        constexpr buffer_view(dynamic_buffer<U>& buffer) noexcept
            : m_data(buffer.data())
            , m_size(buffer.capacity())
        {
        }

        /**
         * @brief Constructs a read-only view over a dynamic_buffer.
         *
         * Enabled only for buffer_view<const T>. Binds to both const and
         * non-const dynamic_buffer instances.
         */
        template<class U>
            requires(std::is_const_v<T> && std::is_same_v<value_type, U>)
        constexpr buffer_view(const dynamic_buffer<U>& buffer) noexcept
            : m_data(buffer.data())
            , m_size(buffer.capacity())
        {
        }

        /**
         * @brief Converting constructor: buffer_view<U> -> buffer_view<const U>.
         *
         * Allows an implicit read-only view of an existing mutable view,
         * analogous to std::span<T> -> std::span<const T>.
         */
        template<class U>
            requires(std::is_const_v<T> && std::is_same_v<std::remove_const_t<T>, U>)
        constexpr buffer_view(const buffer_view<U>& other) noexcept
            : m_data(other.data())
            , m_size(other.size())
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
         * @brief Copies elements from the source array into the buffer.
         *
         * Available only for buffer_view<T> with non-const T.
         *
         * @param src Pointer to the source data.
         * @param count Number of elements to copy.
         * @param offset Starting offset in the destination buffer (in elements).
         * @return The number of elements actually copied.
         */
        constexpr size_t copy_from(const void* src, size_t count = 1, size_t offset = 0) const noexcept
            requires(!std::is_const_v<T>)
        {
            TAV_ASSERT(offset + count <= m_size);
            std::memcpy(m_data + offset, src, count * sizeof(T));
            return count;
        }

        /**
         * @brief Fills a portion of the buffer with a specified value.
         *
         * Available only for buffer_view<T> with non-const T.
         *
         * @param offset Starting index.
         * @param count Number of elements to fill.
         * @param value The value to assign to each element.
         */
        constexpr void fill(size_t offset, size_t count, const T& value) const noexcept
            requires(!std::is_const_v<T>)
        {
            TAV_ASSERT(offset + count <= m_size);
            std::fill_n(m_data + offset, count, value);
        }

        /**
         * @brief Fills the entire buffer with the specified value.
         *
         * Available only for buffer_view<T> with non-const T.
         */
        constexpr void fill(const T& value) const noexcept
            requires(!std::is_const_v<T>)
        {
            std::fill_n(m_data, m_size, value);
        }

        /**
         * @brief Creates a view starting at the specified offset.
         */
        [[nodiscard]] constexpr buffer_view<T> slice(size_t offset) const noexcept
        {
            TAV_ASSERT(offset <= m_size);
            return buffer_view<T>(m_data + offset, m_size - offset);
        }

        /**
         * @brief Creates a view of a subrange of the buffer.
         */
        [[nodiscard]] constexpr buffer_view<T> slice(size_t offset, size_t count) const noexcept
        {
            TAV_ASSERT(offset + count <= m_size);
            return buffer_view<T>(m_data + offset, count);
        }

        /**
         * @brief Returns a pointer to the beginning of the buffer.
         *
         * For buffer_view<const T> this yields a const T*.
         */
        [[nodiscard]] constexpr pointer data() const noexcept
        {
            return m_data;
        }

        /**
         * @brief Accesses an element by index.
         * @note Behavior is undefined if index >= size().
         */
        [[nodiscard]] constexpr reference operator[](size_t index) const noexcept
        {
            TAV_ASSERT(index < m_size);
            return m_data[index];
        }

        /**
         * @brief Allows checking for empty.
         */
        [[nodiscard]] explicit constexpr operator bool() const noexcept
        {
            return m_size != 0;
        }

        [[nodiscard]] constexpr iterator begin() const noexcept
        {
            return m_data;
        }

        [[nodiscard]] constexpr iterator end() const noexcept
        {
            return m_data + m_size;
        }

    private:
        T*     m_data = nullptr;
        size_t m_size = 0;
    };

    /// Read-only view over elements of type T (buffer_view<const T>).
    template<class T>
    using const_buffer_view = buffer_view<const std::remove_const_t<T>>;

    /// Read/write view over elements of type T (buffer_view<T> without const).
    template<class T>
    using mutable_buffer_view = buffer_view<std::remove_const_t<T>>;

} // namespace tavros::core
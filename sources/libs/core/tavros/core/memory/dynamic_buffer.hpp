#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/memory/raw_ptr.hpp>

#include <cstring>
#include <type_traits>
#include <algorithm>

namespace tavros::core
{

    /**
     * @brief Uninitialized resizable buffer for trivial (POD-like) types.
     *
     * Designed for passing raw blob data - audio, geometry, network packets, etc.
     * Unlike std::vector, memory is never zero-initialized on allocation or resize.
     * Uses realloc() for potentially in-place growth.
     *
     * Constraints:
     * - T must be trivially copyable.
     * - No construction or destruction of elements is performed.
     */
    template<class T>
        requires std::is_trivially_copyable_v<T>
    class dynamic_buffer
    {
    public:
        using value_type = T;
        using size_type = size_t;

        /** @brief Constructs an empty buffer. */
        dynamic_buffer() noexcept = default;

        /** @brief Constructs an empty buffer from nullptr. */
        explicit dynamic_buffer(std::nullptr_t) noexcept
        {
        }

        /**
         * @brief Allocates an uninitialized buffer of `capacity` elements.
         * @param capacity Number of elements to allocate. Memory is NOT initialized.
         */
        explicit dynamic_buffer(size_type capacity)
        {
            resize(capacity);
        }

        /**
         * @brief Allocates a buffer and fills it with `count` copies of `value`.
         * @param count Number of elements.
         * @param value Value to fill with.
         */
        dynamic_buffer(size_type count, const value_type& value)
        {
            if (resize(count)) {
                fill(value);
            }
        }

        /**
         * @brief Constructs by copying data from a raw pointer range [begin, end).
         * @param begin Pointer to the first element.
         * @param end   Pointer past the last element.
         */
        dynamic_buffer(const value_type* begin, const value_type* end)
        {
            const auto count = static_cast<ptrdiff_t>(end - begin);
            if (count <= 0) {
                return;
            }
            const auto size = static_cast<size_type>(count);
            if (resize(size)) {
                std::memcpy(m_data.get(), begin, size * sizeof(value_type));
            }
        }

        /**
         * @brief Constructs by copying `count` elements from `src`.
         * @param src   Pointer to source data.
         * @param count Number of elements to copy.
         */
        dynamic_buffer(const value_type* src, size_type count)
            : dynamic_buffer(src, src + count)
        {
        }

        /**
         * @brief Constructs from an initializer list.
         * @param il Initializer list of elements.
         */
        dynamic_buffer(std::initializer_list<value_type> il)
            : dynamic_buffer(il.begin(), il.end())
        {
        }

        /** @brief Copy constructor. Allocates new storage and copies all elements. */
        dynamic_buffer(const dynamic_buffer& other)
        {
            if (other.m_capacity == 0) {
                return;
            }
            if (resize(other.m_capacity)) {
                std::memcpy(m_data.get(), other.m_data.get(), other.m_capacity * sizeof(value_type));
            }
        }

        /** @brief Move constructor. Transfers ownership of storage. */
        dynamic_buffer(dynamic_buffer&& other) noexcept
            : m_data(std::move(other.m_data))
            , m_capacity(other.m_capacity)
        {
            other.m_capacity = 0;
        }

        /** @brief Destructor. Releases allocated memory. */
        ~dynamic_buffer() noexcept
        {
            destroy_storage();
        }

        /** @brief Copy assignment. Reallocates if needed and copies all elements. */
        dynamic_buffer& operator=(const dynamic_buffer& other)
        {
            if (this == &other) {
                return *this;
            }
            if (other.m_capacity == 0) {
                destroy_storage();
                return *this;
            }
            if (resize(other.m_capacity)) {
                std::memcpy(m_data.get(), other.m_data.get(), other.m_capacity * sizeof(value_type));
            }
            return *this;
        }

        /** @brief Move assignment. Releases current storage and takes ownership. */
        dynamic_buffer& operator=(dynamic_buffer&& other) noexcept
        {
            if (this != &other) {
                destroy_storage();
                m_data = std::move(other.m_data);
                m_capacity = other.m_capacity;
                other.m_capacity = 0;
            }
            return *this;
        }

        /** @brief Returns a element reference to element at `index`. */
        value_type& operator[](size_type index) noexcept
        {
            TAV_ASSERT(index < m_capacity);
            return m_data.get()[index];
        }

        /** @brief Returns a const element reference to element at `index`. */
        const value_type& operator[](size_type index) const noexcept
        {
            TAV_ASSERT(index < m_capacity);
            return m_data.get()[index];
        }

        /**
         * @brief Ensures the buffer holds at least `required_capacity` elements.
         *        No-op if current capacity is sufficient.
         */
        bool reserve(size_type required_capacity)
        {
            if (m_capacity >= required_capacity) {
                return true;
            }
            return resize(required_capacity);
        }

        /**
         * @brief Resizes the buffer to exactly `new_capacity` elements.
         *
         * Uses realloc() internally - may extend allocation in-place.
         * Existing data is preserved up to min(old, new) capacity.
         * New memory is NOT initialized.
         * On failure, the buffer is left unchanged.
         *
         * @param new_capacity Number of elements to allocate.
         * @return true on success, false if allocation failed.
         */
        bool resize(size_type new_capacity)
        {
            if (m_capacity == new_capacity) {
                return true;
            }

            value_type* new_data = static_cast<value_type*>(std::realloc(m_data.get(), new_capacity * sizeof(value_type)));
            if (!new_data && new_capacity > 0) {
                return false;
            }

            m_data = new_data;
            m_capacity = new_capacity;
            return true;
        }

        /** @brief Returns the number of allocated elements. */
        [[nodiscard]] size_type capacity() const noexcept
        {
            return m_capacity;
        }

        /** @brief Returns the total size of allocated memory in bytes. */
        [[nodiscard]] size_type size_bytes() const noexcept
        {
            return m_capacity * sizeof(value_type);
        }

        /** @brief Returns true if the buffer has no allocated memory. */
        [[nodiscard]] bool empty() const noexcept
        {
            return m_capacity == 0;
        }

        /**
         * @brief Copies `count` elements from `src` into the buffer at `offset`.
         * @param src    Source data pointer.
         * @param count  Number of elements to copy.
         * @param offset Destination offset in elements.
         * @return Number of elements copied.
         */
        size_type copy_from(const value_type* src, size_type count, size_type offset = 0)
        {
            TAV_ASSERT(src);
            TAV_ASSERT(offset + count <= m_capacity);
            std::memcpy(m_data.get() + offset, src, count * sizeof(value_type));
            return count;
        }

        /**
         * @brief Fills elements in range [offset, offset + count) with `value`.
         * @param offset Starting index.
         * @param count  Number of elements to fill.
         * @param value  Value to assign.
         */
        void fill(size_type offset, size_type count, const value_type& value) noexcept
        {
            TAV_ASSERT(offset + count <= m_capacity);
            std::fill_n(m_data.get() + offset, count, value);
        }

        /** @brief Fills the entire buffer with `value`. */
        void fill(const value_type& value) noexcept
        {
            std::fill_n(m_data.get(), m_capacity, value);
        }

        /**
         * @brief Sets all bytes in the buffer to zero.
         *
         * This performs a raw memory memset and does not invoke
         * any constructors or assignment operators.
         */
        void zero() noexcept
        {
            std::memset(m_data.get(), 0, m_capacity * sizeof(value_type));
        }

        /** @brief Returns a pointer to the underlying buffer memory. */
        [[nodiscard]] value_type* data() noexcept
        {
            return m_data.get();
        }

        /** @brief Returns a const pointer to the underlying buffer memory. */
        [[nodiscard]] const value_type* data() const noexcept
        {
            return m_data.get();
        }

        /** @brief Returns a pointer to the first element. */
        [[nodiscard]] value_type* begin() noexcept
        {
            return m_data.get();
        }

        /** @brief Returns a pointer past the last element. */
        [[nodiscard]] value_type* end() noexcept
        {
            return m_data.get() + m_capacity;
        }

        /** @brief Returns a pointer to the first const element. */
        [[nodiscard]] const value_type* begin() const noexcept
        {
            return m_data.get();
        }

        /** @brief Returns a pointer past the last const element. */
        [[nodiscard]] const value_type* end() const noexcept
        {
            return m_data.get() + m_capacity;
        }

        /** @brief Returns a pointer to the first const element. */
        [[nodiscard]] const value_type* cbegin() const noexcept
        {
            return begin();
        }

        /** @brief Returns a pointer past the last const element. */
        [[nodiscard]] const value_type* cend() const noexcept
        {
            return end();
        }

    private:
        /** @brief Frees the allocated memory. */
        void destroy_storage()
        {
            if (m_data) {
                std::free(m_data.get());
                m_data = nullptr;
                m_capacity = 0;
            }
        }

    private:
        raw_ptr<value_type> m_data = nullptr;
        size_type           m_capacity = 0;
    };

} // namespace tavros::core

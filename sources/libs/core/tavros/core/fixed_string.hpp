#pragma once

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>

#include <fmt/fmt.hpp>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <stdexcept>

namespace tavros::core
{

    /**
     * @brief Fixed-capacity, null-terminated string with STL-like semantics.
     *
     * This class provides a string container with compile-time fixed storage capacity.
     * It behaves similarly to @c std::basic_string, but does not perform dynamic memory
     * allocation. All data is stored inline within the object.
     *
     * The string is always null-terminated, and its maximum usable size is
     * @c Capacity - 1 characters (the last element is reserved for the null terminator).
     *
     * The interface follows STL conventions where applicable:
     * - value semantics (copy/move operations behave like standard containers),
     * - iterator and pointer types compatible with standard algorithms,
     * - construction and assignment patterns similar to @c std::basic_string,
     * - use of @c traits_type for character operations.
     *
     * Unlike @c std::basic_string:
     * - capacity is fixed at compile time and cannot grow,
     * - operations that exceed capacity rely on assertions or explicit policies,
     * - no dynamic allocation is performed.
     *
     * Requirements on @p Char:
     * - must match @c Traits::char_type,
     * - must be trivially copyable and trivially default constructible,
     * - must have standard layout,
     * - must not be an array type.
     *
     * @tparam Capacity Total storage size including space for the null terminator.
     *                  Must be greater than 0 and a multiple of 8.
     * @tparam Char Character type.
     * @tparam Traits Character traits type (defaults to @c std::char_traits<Char>).
     *
     * @note
     * The effective maximum number of stored characters is available via
     * @ref static_capacity.
     *
     * @warning
     * Exceeding capacity or violating preconditions results in undefined behavior
     * in release builds (assertions are used in debug).
     */
    template<size_t Capacity, class Char, class Traits = std::char_traits<Char>>
    class basic_fixed_string
    {
        static_assert(Capacity > 0, "Capacity must be greater than zero");
        static_assert(std::is_same_v<Char, typename Traits::char_type>);
        static_assert(
            !std::is_array_v<Char>
            && std::is_trivially_copyable_v<Char>
            && std::is_trivially_default_constructible_v<Char>
            && std::is_standard_layout_v<Char>
        );

    public:
        using traits_type = Traits;
        using value_type = Char;
        using size_type = size_t;
        using smallest_size_type = smallest_size_t<Capacity>;
        using difference_type = ptrdiff_t;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;
        using iterator = value_type*;
        using const_iterator = const value_type*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        static constexpr size_type npos = static_cast<size_type>(-1);

        static constexpr size_type static_capacity = Capacity - 1;

    private:
        using view_type = basic_string_view<Char, Traits>;

        [[nodiscard]] constexpr view_type as_view() const noexcept
        {
            return view_type{m_data, size()};
        }

    public:
        // ----------------------------------------------------------------------
        // Constructors
        // ----------------------------------------------------------------------

        constexpr basic_fixed_string() noexcept
            : m_size(0)
        {
            m_data[0] = Char{};
        }

        constexpr basic_fixed_string(const Char* s)
            : basic_fixed_string(s, Traits::length(s))
        {
        }

        constexpr basic_fixed_string(const Char* s, size_type count)
            : m_size(count)
        {
            TAV_ASSERT(count < Capacity);
            Traits::copy(m_data, s, count);
            m_data[size()] = Char{};
        }

        constexpr basic_fixed_string(size_type count, Char ch)
            : m_size(count)
        {
            TAV_ASSERT(count < Capacity);
            Traits::assign(m_data, count, ch);
            m_data[size()] = Char{};
        }

        template<class InputIt>
        constexpr basic_fixed_string(InputIt first, InputIt last)
            : m_size(static_cast<size_type>(std::distance(first, last)))
        {
            TAV_ASSERT(size() < Capacity);
            std::copy(first, last, m_data);
            m_data[size()] = Char{};
        }

        constexpr basic_fixed_string(std::initializer_list<Char> list)
            : basic_fixed_string(list.begin(), static_cast<size_type>(list.size()))
        {
        }

        explicit constexpr basic_fixed_string(view_type sv)
            : basic_fixed_string(sv.data(), sv.size())
        {
        }

        constexpr basic_fixed_string(const basic_fixed_string&) noexcept = default;
        constexpr basic_fixed_string(basic_fixed_string&&) noexcept = default;

        constexpr basic_fixed_string& operator=(const basic_fixed_string&) noexcept = default;
        constexpr basic_fixed_string& operator=(basic_fixed_string&&) noexcept = default;

        constexpr ~basic_fixed_string() noexcept = default;


        // ----------------------------------------------------------------------
        // Assignment operators
        // ----------------------------------------------------------------------

        constexpr basic_fixed_string& operator=(const Char* s)
        {
            return assign(s);
        }

        constexpr basic_fixed_string& operator=(Char ch)
        {
            m_size = 1;
            m_data[0] = ch;
            m_data[1] = Char{};
            return *this;
        }

        constexpr basic_fixed_string& operator=(view_type sv)
        {
            return assign(sv);
        }

        constexpr basic_fixed_string& operator=(std::initializer_list<Char> list)
        {
            return assign(list);
        }


        // ----------------------------------------------------------------------
        // Element access
        // ----------------------------------------------------------------------

        [[nodiscard]] constexpr reference operator[](size_type index) noexcept
        {
            TAV_ASSERT(index <= size());
            return m_data[index];
        }

        [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept
        {
            TAV_ASSERT(index <= size());
            return m_data[index];
        }

        [[nodiscard]] constexpr reference at(size_type index)
        {
            if (index >= size()) {
                throw std::out_of_range("basic_fixed_string::at: index out of range");
            }
            return m_data[index];
        }

        [[nodiscard]] constexpr const_reference at(size_type index) const
        {
            if (index >= size()) {
                throw std::out_of_range("basic_fixed_string::at: index out of range");
            }
            return m_data[index];
        }

        [[nodiscard]] constexpr reference front() noexcept
        {
            TAV_ASSERT(size() > 0);
            return m_data[0];
        }

        [[nodiscard]] constexpr const_reference front() const noexcept
        {
            TAV_ASSERT(size() > 0);
            return m_data[0];
        }

        [[nodiscard]] constexpr reference back() noexcept
        {
            TAV_ASSERT(size() > 0);
            return m_data[size() - 1];
        }

        [[nodiscard]] constexpr const_reference back() const noexcept
        {
            TAV_ASSERT(size() > 0);
            return m_data[size() - 1];
        }


        [[nodiscard]] constexpr pointer data() noexcept
        {
            return m_data;
        }

        [[nodiscard]] constexpr const_pointer data() const noexcept
        {
            return m_data;
        }

        [[nodiscard]] constexpr const_pointer c_str() const noexcept
        {
            return m_data;
        }

        // Implicit conversion to string_view
        constexpr operator view_type() const noexcept
        {
            return as_view();
        }

        // ----------------------------------------------------------------------
        // Capacity
        // ----------------------------------------------------------------------

        [[nodiscard]] constexpr size_type size() const noexcept
        {
            return static_cast<size_type>(m_size);
        }

        [[nodiscard]] constexpr size_type length() const noexcept
        {
            return static_cast<size_type>(m_size);
        }

        [[nodiscard]] constexpr size_type max_size() const noexcept
        {
            return Capacity - 1;
        }

        [[nodiscard]] constexpr size_type capacity() const noexcept
        {
            return Capacity - 1;
        }

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return size() == 0;
        }

        [[nodiscard]] constexpr bool full() const noexcept
        {
            return size() >= Capacity - 1;
        }

        [[nodiscard]] constexpr size_type remaining() const noexcept
        {
            return Capacity - 1 - size();
        }


        // ----------------------------------------------------------------------
        // Iterators
        // ----------------------------------------------------------------------

        [[nodiscard]] constexpr iterator begin() noexcept
        {
            return m_data;
        }

        [[nodiscard]] constexpr const_iterator begin() const noexcept
        {
            return m_data;
        }

        [[nodiscard]] constexpr const_iterator cbegin() const noexcept
        {
            return m_data;
        }


        [[nodiscard]] constexpr iterator end() noexcept
        {
            return m_data + size();
        }

        [[nodiscard]] constexpr const_iterator end() const noexcept
        {
            return m_data + size();
        }

        [[nodiscard]] constexpr const_iterator cend() const noexcept
        {
            return m_data + size();
        }


        [[nodiscard]] constexpr reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }


        [[nodiscard]] constexpr reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept
        {
            return const_reverse_iterator(begin());
        }


        // ----------------------------------------------------------------------
        // Modifiers - set_size / clear / push_back / pop_back / resize / swap
        // ----------------------------------------------------------------------

        /**
         * @brief Sets the string size manually after writing into the buffer via @ref data().
         *
         * This function is intended for advanced use cases where the underlying buffer is
         * modified directly. It assumes that the caller maintains all required invariants.
         *
         * The @ref tavros::core::unsafe_t tag must be provided to explicitly acknowledge
         * the unsafe nature of this operation.
         *
         * Example:
         * @code
         * char* buf = s.data();
         * size_t n  = ::recv(sock, buf, s.capacity(), 0);
         * s.set_size(tavros::unsafe, n);
         * @endcode
         *
         * @param[in] tavros::unsafe_t Tag indicating that safety checks are intentionally bypassed.
         * @param[in] new_size New logical size of the string (excluding null terminator).
         *
         * @pre (not enforced in release builds):
         * - new_size < Capacity
         * - data()[new_size] == Char{} (buffer must remain null-terminated)
         *
         * @warning
         * Violating the preconditions results in undefined behavior.
         */
        constexpr void set_size(unsafe_t, size_type size) noexcept
        {
            TAV_ASSERT(size < Capacity);
            TAV_ASSERT(m_data[size] == Char{});
            m_size = size;
        }

        constexpr void clear() noexcept
        {
            m_size = 0;
            m_data[0] = Char{};
        }

        constexpr void push_back(Char ch)
        {
            TAV_ASSERT(size() + 1 < Capacity);
            m_data[m_size++] = ch;
            m_data[m_size] = Char{};
        }

        constexpr void pop_back() noexcept
        {
            TAV_ASSERT(size() > 0);
            if (size() > 0) {
                m_data[--m_size] = Char{};
            }
        }

        constexpr void resize(size_type count, Char ch = Char{})
        {
            if (count <= size()) {
                m_size = static_cast<smallest_size_type>(count);
                m_data[size()] = Char{};
            } else {
                TAV_ASSERT(count < Capacity);
                Traits::assign(m_data + size(), count - size(), ch);
                m_size = static_cast<smallest_size_type>(count);
                m_data[size()] = Char{};
            }
        }

        constexpr void swap(basic_fixed_string& other) noexcept
        {
            // Both objects live on the stack - swap via a local buffer
            Char      tmp[Capacity];
            size_type tmp_size = other.size();
            Traits::copy(tmp, other.m_data, other.size() + 1);
            Traits::copy(other.m_data, m_data, size() + 1);
            other.m_size = m_size;
            Traits::copy(m_data, tmp, tmp_size + 1);
            m_size = tmp_size;
        }


        // ----------------------------------------------------------------------
        // assign
        // ----------------------------------------------------------------------

        constexpr basic_fixed_string& assign(const Char* s, size_type count)
        {
            TAV_ASSERT(count < Capacity);
            m_size = static_cast<smallest_size_t>(count);
            Traits::copy(m_data, s, count);
            m_data[size()] = Char{};
            return *this;
        }

        constexpr basic_fixed_string& assign(const Char* s)
        {
            return assign(s, Traits::length(s));
        }

        constexpr basic_fixed_string& assign(size_type count, Char ch)
        {
            TAV_ASSERT(count < Capacity);
            m_size = static_cast<smallest_size_t>(count);
            Traits::assign(m_data, count, ch);
            m_data[size()] = Char{};
            return *this;
        }

        constexpr basic_fixed_string& assign(view_type sv)
        {
            return assign(sv.data(), sv.size());
        }

        constexpr basic_fixed_string& assign(view_type sv, size_type pos, size_type count = npos)
        {
            return assign(sv.substr(pos, count));
        }

        constexpr basic_fixed_string& assign(const basic_fixed_string& other)
        {
            if (this != &other) {
                *this = other;
            }
            return *this;
        }

        constexpr basic_fixed_string& assign(const basic_fixed_string& other, size_type pos, size_type count = npos)
        {
            return assign(view_type{other}.substr(pos, count));
        }

        constexpr basic_fixed_string& assign(std::initializer_list<Char> list)
        {
            return assign(list.begin(), static_cast<size_type>(list.size()));
        }

        template<class InputIt>
        constexpr basic_fixed_string& assign(InputIt first, InputIt last)
        {
            const auto count = static_cast<size_type>(std::distance(first, last));
            TAV_ASSERT(count < Capacity);
            m_size = static_cast<smallest_size_type>(count);
            std::copy(first, last, m_data);
            m_data[size()] = Char{};
            return *this;
        }


        // ----------------------------------------------------------------------
        // append
        // ----------------------------------------------------------------------

        constexpr basic_fixed_string& append(const Char* s, size_type count)
        {
            TAV_ASSERT(size() + count < Capacity);
            Traits::copy(m_data + size(), s, count);
            m_size += static_cast<smallest_size_type>(count);
            m_data[size()] = Char{};
            return *this;
        }

        constexpr basic_fixed_string& append(const Char* s)
        {
            return append(s, Traits::length(s));
        }

        constexpr basic_fixed_string& append(size_type count, Char ch)
        {
            TAV_ASSERT(size() + count < Capacity);
            Traits::assign(m_data + size(), count, ch);
            m_size += count;
            m_data[size()] = Char{};
            return *this;
        }

        constexpr basic_fixed_string& append(view_type sv)
        {
            return append(sv.data(), sv.size());
        }

        constexpr basic_fixed_string& append(view_type sv, size_type pos, size_type count = npos)
        {
            return append(sv.substr(pos, count));
        }

        constexpr basic_fixed_string& append(const basic_fixed_string& other)
        {
            return append(other.m_data, other.size());
        }

        constexpr basic_fixed_string& append(
            const basic_fixed_string& other, size_type pos, size_type count = npos
        )
        {
            return append(view_type{other}.substr(pos, count));
        }

        constexpr basic_fixed_string& append(std::initializer_list<Char> list)
        {
            return append(list.begin(), static_cast<size_type>(list.size()));
        }

        template<class InputIt>
        constexpr basic_fixed_string& append(InputIt first, InputIt last)
        {
            const auto count = static_cast<size_type>(std::distance(first, last));
            TAV_ASSERT(size() + count < Capacity);
            std::copy(first, last, m_data + size());
            m_size += static_cast<smallest_size_type>(count);
            m_data[size()] = Char{};
            return *this;
        }


        // ----------------------------------------------------------------------
        // operator+=
        // ----------------------------------------------------------------------

        constexpr basic_fixed_string& operator+=(const basic_fixed_string& other)
        {
            return append(other);
        }

        constexpr basic_fixed_string& operator+=(Char ch)
        {
            push_back(ch);
            return *this;
        }

        constexpr basic_fixed_string& operator+=(const Char* s)
        {
            return append(s);
        }

        constexpr basic_fixed_string& operator+=(view_type sv)
        {
            return append(sv);
        }

        constexpr basic_fixed_string& operator+=(std::initializer_list<Char> list)
        {
            return append(list);
        }

        // ----------------------------------------------------------------------
        // operator/=
        // ----------------------------------------------------------------------

        constexpr basic_fixed_string& operator/=(const basic_fixed_string& other)
        {
            append("/");
            return append(other);
        }

        constexpr basic_fixed_string& operator/=(Char ch)
        {
            append("/");
            push_back(ch);
            return *this;
        }

        constexpr basic_fixed_string& operator/=(const Char* s)
        {
            append("/");
            return append(s);
        }

        constexpr basic_fixed_string& operator/=(view_type sv)
        {
            append("/");
            return append(sv);
        }

        constexpr basic_fixed_string& operator/=(std::initializer_list<Char> list)
        {
            append("/");
            return append(list);
        }

        // ----------------------------------------------------------------------
        // insert
        // ----------------------------------------------------------------------

        constexpr basic_fixed_string& insert(size_type pos, const Char* s, size_type count)
        {
            TAV_ASSERT(pos <= size() && size() + count < Capacity);
            Traits::move(m_data + pos + count, m_data + pos, size() - pos + 1); // shift suffix + null
            Traits::copy(m_data + pos, s, count);
            m_size += static_cast<smallest_size_type>(count);
            return *this;
        }

        constexpr basic_fixed_string& insert(size_type pos, const Char* s)
        {
            return insert(pos, s, Traits::length(s));
        }

        constexpr basic_fixed_string& insert(size_type pos, size_type count, Char ch)
        {
            TAV_ASSERT(pos <= size() && size() + count < Capacity);
            Traits::move(m_data + pos + count, m_data + pos, size() - pos + 1);
            Traits::assign(m_data + pos, count, ch);
            m_size += static_cast<smallest_size_type>(count);
            return *this;
        }

        constexpr basic_fixed_string& insert(size_type pos, view_type sv)
        {
            return insert(pos, sv.data(), sv.size());
        }

        constexpr basic_fixed_string& insert(size_type pos, view_type sv, size_type sv_pos, size_type count = npos)
        {
            return insert(pos, sv.substr(sv_pos, count));
        }

        constexpr basic_fixed_string& insert(size_type pos, const basic_fixed_string& other)
        {
            return insert(pos, other.m_data, other.size());
        }

        constexpr basic_fixed_string& insert(size_type pos, const basic_fixed_string& other, size_type other_pos, size_type count = npos)
        {
            return insert(pos, view_type{other}.substr(other_pos, count));
        }

        constexpr iterator insert(const_iterator where, Char ch)
        {
            const auto pos = static_cast<size_type>(where - m_data);
            insert(pos, 1, ch);
            return m_data + pos;
        }

        constexpr iterator insert(const_iterator where, size_type count, Char ch)
        {
            const auto pos = static_cast<size_type>(where - m_data);
            insert(pos, count, ch);
            return m_data + pos;
        }

        constexpr iterator insert(const_iterator where, std::initializer_list<Char> list)
        {
            const auto pos = static_cast<size_type>(where - m_data);
            insert(pos, list.begin(), static_cast<size_type>(list.size()));
            return m_data + pos;
        }

        template<class InputIt>
        constexpr iterator insert(const_iterator where, InputIt first, InputIt last)
        {
            const auto pos = static_cast<size_type>(where - m_data);
            const auto count = static_cast<size_type>(std::distance(first, last));
            TAV_ASSERT(pos <= size() && size() + count < Capacity);
            Traits::move(m_data + pos + count, m_data + pos, size() - pos + 1);
            std::copy(first, last, m_data + pos);
            m_size += static_cast<smallest_size_type>(count);
            return m_data + pos;
        }


        // ----------------------------------------------------------------------
        // erase
        // ----------------------------------------------------------------------

        constexpr basic_fixed_string& erase(size_type pos = 0, size_type count = npos)
        {
            TAV_ASSERT(pos <= size());
            const size_type actual = (count < size() - pos ? count : size() - pos);
            Traits::move(m_data + pos, m_data + pos + actual, size() - pos - actual + 1);
            m_size -= static_cast<smallest_size_type>(actual);
            return *this;
        }

        constexpr iterator erase(const_iterator where) noexcept
        {
            const auto pos = static_cast<size_type>(where - m_data);
            erase(pos, 1);
            return m_data + pos;
        }

        constexpr iterator erase(const_iterator first, const_iterator last) noexcept
        {
            const auto pos = static_cast<size_type>(first - m_data);
            const auto count = static_cast<size_type>(last - first);
            erase(pos, count);
            return m_data + pos;
        }


        // ----------------------------------------------------------------------
        // replace
        // ----------------------------------------------------------------------

        constexpr basic_fixed_string& replace(size_type pos, size_type count, const Char* s, size_type s_count)
        {
            TAV_ASSERT(pos <= size());
            const size_type actual = (count < size() - pos ? count : size() - pos);
            const size_type new_size = size() - actual + s_count;
            TAV_ASSERT(new_size < Capacity);
            Traits::move(m_data + pos + s_count, m_data + pos + actual, size() - pos - actual + 1);
            Traits::copy(m_data + pos, s, s_count);
            m_size = static_cast<smallest_size_type>(new_size);
            return *this;
        }

        constexpr basic_fixed_string& replace(size_type pos, size_type count, const Char* s)
        {
            return replace(pos, count, s, Traits::length(s));
        }

        constexpr basic_fixed_string& replace(size_type pos, size_type count, size_type n, Char ch)
        {
            TAV_ASSERT(pos <= size());
            const size_type actual = (count < size() - pos ? count : size() - pos);
            const size_type new_size = size() - actual + n;
            TAV_ASSERT(new_size < Capacity);
            Traits::move(m_data + pos + n, m_data + pos + actual, size() - pos - actual + 1);
            Traits::assign(m_data + pos, n, ch);
            m_size = static_cast<smallest_size_type>(new_size);
            return *this;
        }

        constexpr basic_fixed_string& replace(size_type pos, size_type count, view_type sv)
        {
            return replace(pos, count, sv.data(), sv.size());
        }

        constexpr basic_fixed_string& replace(size_type pos, size_type count, view_type sv, size_type sv_pos, size_type sv_count = npos)
        {
            return replace(pos, count, sv.substr(sv_pos, sv_count));
        }

        constexpr basic_fixed_string& replace(size_type pos, size_type count, const basic_fixed_string& other)
        {
            return replace(pos, count, other.m_data, other.size());
        }

        constexpr basic_fixed_string& replace(size_type pos, size_type count, const basic_fixed_string& other, size_type other_pos, size_type other_count = npos)
        {
            return replace(pos, count, view_type{other}.substr(other_pos, other_count));
        }

        // Iterator-based replace overloads delegate to index-based ones
        constexpr basic_fixed_string& replace(const_iterator first, const_iterator last, const Char* s, size_type count)
        {
            return replace(
                static_cast<size_type>(first - m_data),
                static_cast<size_type>(last - first), s, count
            );
        }

        constexpr basic_fixed_string& replace(const_iterator first, const_iterator last, const Char* s)
        {
            return replace(
                static_cast<size_type>(first - m_data),
                static_cast<size_type>(last - first), s
            );
        }

        constexpr basic_fixed_string& replace(const_iterator first, const_iterator last, size_type count, Char ch)
        {
            return replace(
                static_cast<size_type>(first - m_data),
                static_cast<size_type>(last - first), count, ch
            );
        }

        constexpr basic_fixed_string& replace(const_iterator first, const_iterator last, view_type sv)
        {
            return replace(
                static_cast<size_type>(first - m_data),
                static_cast<size_type>(last - first), sv
            );
        }

        constexpr basic_fixed_string& replace(const_iterator first, const_iterator last, const basic_fixed_string& other)
        {
            return replace(
                static_cast<size_type>(first - m_data),
                static_cast<size_type>(last - first), other
            );
        }

        constexpr basic_fixed_string& replace(const_iterator first, const_iterator last, std::initializer_list<Char> list)
        {
            return replace(
                static_cast<size_type>(first - m_data),
                static_cast<size_type>(last - first),
                list.begin(), static_cast<size_type>(list.size())
            );
        }

        template<class InputIt>
        constexpr basic_fixed_string& replace(const_iterator first, const_iterator last, InputIt s_first, InputIt s_last)
        {
            const basic_fixed_string tmp(s_first, s_last);
            return replace(first, last, tmp.m_data, tmp.size());
        }


        // ----------------------------------------------------------------------
        // copy / substr
        // ----------------------------------------------------------------------

        constexpr size_type copy(Char* dest, size_type count, size_type pos = 0) const
        {
            TAV_ASSERT(pos <= size());
            const size_type actual = (count < size() - pos ? count : size() - pos);
            Traits::copy(dest, m_data + pos, actual);
            return actual;
        }

        [[nodiscard]] constexpr basic_fixed_string substr(size_type pos = 0, size_type count = npos) const
        {
            return basic_fixed_string{as_view().substr(pos, count)};
        }


        // ----------------------------------------------------------------------
        // find / rfind / find_first_of / find_last_of / find_first_not_of / find_last_not_of - delegate to string_view
        // ----------------------------------------------------------------------

        [[nodiscard]] constexpr size_type find(view_type sv, size_type pos = 0) const noexcept
        {
            return as_view().find(sv, pos);
        }

        [[nodiscard]] constexpr size_type find(const Char* s, size_type pos, size_type count) const noexcept
        {
            return as_view().find(s, pos, count);
        }

        [[nodiscard]] constexpr size_type find(const Char* s, size_type pos = 0) const noexcept
        {
            return as_view().find(s, pos);
        }

        [[nodiscard]] constexpr size_type find(Char ch, size_type pos = 0) const noexcept
        {
            return as_view().find(ch, pos);
        }

        [[nodiscard]] constexpr size_type find(const basic_fixed_string& other, size_type pos = 0) const noexcept
        {
            return as_view().find(view_type{other}, pos);
        }


        [[nodiscard]] constexpr size_type rfind(view_type sv, size_type pos = npos) const noexcept
        {
            return as_view().rfind(sv, pos);
        }

        [[nodiscard]] constexpr size_type rfind(const Char* s, size_type pos, size_type count) const noexcept
        {
            return as_view().rfind(s, pos, count);
        }

        [[nodiscard]] constexpr size_type rfind(const Char* s, size_type pos = npos) const noexcept
        {
            return as_view().rfind(s, pos);
        }

        [[nodiscard]] constexpr size_type rfind(Char ch, size_type pos = npos) const noexcept
        {
            return as_view().rfind(ch, pos);
        }

        [[nodiscard]] constexpr size_type rfind(const basic_fixed_string& other, size_type pos = npos) const noexcept
        {
            return as_view().rfind(view_type{other}, pos);
        }


        [[nodiscard]] constexpr size_type find_first_of(view_type sv, size_type pos = 0) const noexcept
        {
            return as_view().find_first_of(sv, pos);
        }

        [[nodiscard]] constexpr size_type find_first_of(const Char* s, size_type pos, size_type count) const noexcept
        {
            return as_view().find_first_of(s, pos, count);
        }

        [[nodiscard]] constexpr size_type find_first_of(const Char* s, size_type pos = 0) const noexcept
        {
            return as_view().find_first_of(s, pos);
        }

        [[nodiscard]] constexpr size_type find_first_of(Char ch, size_type pos = 0) const noexcept
        {
            return as_view().find_first_of(ch, pos);
        }

        [[nodiscard]] constexpr size_type find_first_of(const basic_fixed_string& other, size_type pos = 0) const noexcept
        {
            return as_view().find_first_of(view_type{other}, pos);
        }


        [[nodiscard]] constexpr size_type find_last_of(view_type sv, size_type pos = npos) const noexcept
        {
            return as_view().find_last_of(sv, pos);
        }

        [[nodiscard]] constexpr size_type find_last_of(const Char* s, size_type pos, size_type count) const noexcept
        {
            return as_view().find_last_of(s, pos, count);
        }

        [[nodiscard]] constexpr size_type find_last_of(const Char* s, size_type pos = npos) const noexcept
        {
            return as_view().find_last_of(s, pos);
        }

        [[nodiscard]] constexpr size_type find_last_of(Char ch, size_type pos = npos) const noexcept
        {
            return as_view().find_last_of(ch, pos);
        }

        [[nodiscard]] constexpr size_type find_last_of(const basic_fixed_string& other, size_type pos = npos) const noexcept
        {
            return as_view().find_last_of(view_type{other}, pos);
        }


        [[nodiscard]] constexpr size_type find_first_not_of(view_type sv, size_type pos = 0) const noexcept
        {
            return as_view().find_first_not_of(sv, pos);
        }

        [[nodiscard]] constexpr size_type find_first_not_of(const Char* s, size_type pos, size_type count) const noexcept
        {
            return as_view().find_first_not_of(s, pos, count);
        }

        [[nodiscard]] constexpr size_type find_first_not_of(const Char* s, size_type pos = 0) const noexcept
        {
            return as_view().find_first_not_of(s, pos);
        }

        [[nodiscard]] constexpr size_type find_first_not_of(Char ch, size_type pos = 0) const noexcept
        {
            return as_view().find_first_not_of(ch, pos);
        }

        [[nodiscard]] constexpr size_type find_first_not_of(const basic_fixed_string& other, size_type pos = 0) const noexcept
        {
            return as_view().find_first_not_of(view_type{other}, pos);
        }


        [[nodiscard]] constexpr size_type find_last_not_of(view_type sv, size_type pos = npos) const noexcept
        {
            return as_view().find_last_not_of(sv, pos);
        }

        [[nodiscard]] constexpr size_type find_last_not_of(const Char* s, size_type pos, size_type count) const noexcept
        {
            return as_view().find_last_not_of(s, pos, count);
        }

        [[nodiscard]] constexpr size_type find_last_not_of(const Char* s, size_type pos = npos) const noexcept
        {
            return as_view().find_last_not_of(s, pos);
        }

        [[nodiscard]] constexpr size_type find_last_not_of(Char ch, size_type pos = npos) const noexcept
        {
            return as_view().find_last_not_of(ch, pos);
        }

        [[nodiscard]] constexpr size_type find_last_not_of(const basic_fixed_string& other, size_type pos = npos) const noexcept
        {
            return as_view().find_last_not_of(view_type{other}, pos);
        }


        // ----------------------------------------------------------------------
        // compare - delegates to string_view
        // ----------------------------------------------------------------------

        [[nodiscard]] constexpr int compare(view_type sv) const noexcept
        {
            return as_view().compare(sv);
        }

        [[nodiscard]] constexpr int compare(size_type pos, size_type count, view_type sv) const
        {
            return as_view().substr(pos, count).compare(sv);
        }

        [[nodiscard]] constexpr int compare(size_type pos, size_type count, view_type sv, size_type sv_pos, size_type sv_count = npos) const
        {
            return as_view().substr(pos, count).compare(sv.substr(sv_pos, sv_count));
        }

        [[nodiscard]] constexpr int compare(const Char* s) const noexcept
        {
            return as_view().compare(s);
        }

        [[nodiscard]] constexpr int compare(size_type pos, size_type count, const Char* s) const
        {
            return as_view().substr(pos, count).compare(s);
        }

        [[nodiscard]] constexpr int compare(size_type pos, size_type count, const Char* s, size_type s_count) const
        {
            return as_view().substr(pos, count).compare(view_type{s, s_count});
        }

        [[nodiscard]] constexpr int compare(const basic_fixed_string& other) const noexcept
        {
            return as_view().compare(view_type{other});
        }

        [[nodiscard]] constexpr int compare(size_type pos, size_type count, const basic_fixed_string& other) const
        {
            return as_view().substr(pos, count).compare(view_type{other});
        }

        [[nodiscard]] constexpr int compare(size_type pos, size_type count, const basic_fixed_string& other, size_type other_pos, size_type other_count = npos) const
        {
            return as_view().substr(pos, count).compare(view_type{other}.substr(other_pos, other_count));
        }


        // ----------------------------------------------------------------------
        // starts_with / ends_with / contains - delegate to string_view
        // ----------------------------------------------------------------------

        [[nodiscard]] constexpr bool starts_with(view_type sv) const noexcept
        {
            return as_view().starts_with(sv);
        }

        [[nodiscard]] constexpr bool starts_with(Char ch) const noexcept
        {
            return as_view().starts_with(ch);
        }

        [[nodiscard]] constexpr bool starts_with(const Char* s) const noexcept
        {
            return as_view().starts_with(s);
        }


        [[nodiscard]] constexpr bool ends_with(view_type sv) const noexcept
        {
            return as_view().ends_with(sv);
        }

        [[nodiscard]] constexpr bool ends_with(Char ch) const noexcept
        {
            return as_view().ends_with(ch);
        }

        [[nodiscard]] constexpr bool ends_with(const Char* s) const noexcept
        {
            return as_view().ends_with(s);
        }


        [[nodiscard]] constexpr bool contains(view_type sv) const noexcept
        {
            return as_view().contains(sv);
        }

        [[nodiscard]] constexpr bool contains(Char ch) const noexcept
        {
            return as_view().contains(ch);
        }

        [[nodiscard]] constexpr bool contains(const Char* s) const noexcept
        {
            return as_view().contains(s);
        }

    public:
        template<class... Args>
        [[nodiscard]] static basic_fixed_string format(fmt::format_string<Args...> fmt, Args&&... args)
        {
            basic_fixed_string result;
            auto               r = fmt::format_to_n(
                result.m_data,
                static_cast<std::ptrdiff_t>(Capacity - 1),
                fmt,
                std::forward<Args>(args)...
            );

            if (static_cast<size_type>(r.size) > Capacity - 1) {
                throw std::overflow_error("basic_fixed_string::format: result exceeds capacity");
            }

            result.m_size = static_cast<smallest_size_type>(r.out - result.m_data);
            result.m_data[result.size()] = Char{};
            return result;
        }

        template<class... Args>
        [[nodiscard]] static basic_fixed_string format(on_overflow_truncate_t, fmt::format_string<Args...> fmt, Args&&... args) noexcept
        {
            basic_fixed_string result;
            auto               r = fmt::format_to_n(
                result.m_data,
                static_cast<std::ptrdiff_t>(Capacity - 1),
                fmt,
                std::forward<Args>(args)...
            );

            result.m_size = static_cast<size_type>(r.out - result.m_data);
            result.m_data[result.size()] = Char{};
            return result;
        }

    private:
        smallest_size_type m_size;
        value_type         m_data[Capacity];
    };

    // --------------------------------------------------------------------------
    // Non-member: swap
    // --------------------------------------------------------------------------

    template<size_t N, class Char, class Traits>
    constexpr void swap(basic_fixed_string<N, Char, Traits>& lhs, basic_fixed_string<N, Char, Traits>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    // --------------------------------------------------------------------------
    // Non-member: comparison operators (C++20 - one <=> covers all six)
    // --------------------------------------------------------------------------

    template<size_t N, size_t M, class Char, class Traits>
    [[nodiscard]] constexpr bool operator==(const basic_fixed_string<N, Char, Traits>& lhs, const basic_fixed_string<M, Char, Traits>& rhs) noexcept
    {
        return basic_string_view<Char, Traits>{lhs} == basic_string_view<Char, Traits>{rhs};
    }

    template<size_t N, class Char, class Traits>
    [[nodiscard]] constexpr bool operator==(const basic_fixed_string<N, Char, Traits>& lhs, const Char* rhs) noexcept
    {
        return basic_string_view<Char, Traits>{lhs} == rhs;
    }

    template<size_t N, size_t M, class Char, class Traits>
    [[nodiscard]] constexpr auto operator<=>(const basic_fixed_string<N, Char, Traits>& lhs, const basic_fixed_string<M, Char, Traits>& rhs) noexcept
    {
        return basic_string_view<Char, Traits>{lhs} <=> basic_string_view<Char, Traits>{rhs};
    }

    template<size_t N, class Char, class Traits>
    [[nodiscard]] constexpr auto operator<=>(const basic_fixed_string<N, Char, Traits>& lhs, const Char* rhs) noexcept
    {
        return basic_string_view<Char, Traits>{lhs} <=> basic_string_view<Char, Traits>{rhs};
    }

    // --------------------------------------------------------------------------
    // Non-member: operator+  (result capacity is known at compile time)
    // --------------------------------------------------------------------------

    template<size_t N, size_t M, class Char, class Traits>
    [[nodiscard]] constexpr basic_fixed_string<N + M, Char, Traits> operator+(const basic_fixed_string<N, Char, Traits>& lhs, const basic_fixed_string<M, Char, Traits>& rhs)
    {
        basic_fixed_string<N + M, Char, Traits> result(lhs);
        result.append(rhs);
        return result;
    }

    // --------------------------------------------------------------------------
    // Convenience aliases
    // --------------------------------------------------------------------------

    template<size_t Capacity>
    using fixed_string = basic_fixed_string<Capacity, char>;
    template<size_t Capacity>
    using fixed_wstring = basic_fixed_string<Capacity, wchar_t>;
    template<size_t Capacity>
    using fixed_u8string = basic_fixed_string<Capacity, char8_t>;
    template<size_t Capacity>
    using fixed_u16string = basic_fixed_string<Capacity, char16_t>;
    template<size_t Capacity>
    using fixed_u32string = basic_fixed_string<Capacity, char32_t>;

} // namespace tavros::core

// --------------------------------------------------------------------------
// std::hash specialisation - allows use as unordered_map key
// --------------------------------------------------------------------------

namespace std
{

    template<size_t Capacity, class Char, class Traits>
    struct hash<tavros::core::basic_fixed_string<Capacity, Char, Traits>>
    {
        size_t operator()(const tavros::core::basic_fixed_string<Capacity, Char, Traits>& s) const noexcept
        {
            return hash<tavros::core::basic_string_view<Char, Traits>>{}(s);
        }
    };

} // namespace std

namespace fmt
{
    template<size_t Capacity, class Char, class Traits>
    struct formatter<tavros::core::basic_fixed_string<Capacity, Char, Traits>, Char>
        : fmt::formatter<std::basic_string_view<Char, Traits>, Char>
    {
        template<class FormatContext>
        auto format(const tavros::core::basic_fixed_string<Capacity, Char, Traits>& s, FormatContext& ctx) const
        {
            return fmt::formatter<std::basic_string_view<Char, Traits>, Char>::format(std::basic_string_view<Char, Traits>{s.data(), s.size()}, ctx);
        }
    };
} // namespace fmt

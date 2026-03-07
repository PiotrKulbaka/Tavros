#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/traits.hpp>

#include <iterator>
#include <tuple>

namespace tavros::core
{

    /**
     * @brief Random-access iterator over a subset of typed columns in a table.
     *
     * Provides simultaneous iteration over columns @p Ty... of a @p Table,
     * dereferencing to a tuple of references. Supports both mutable and const
     * variants controlled by @p IsConst.
     *
     * @tparam IsConst  If @c true, yields const references on dereference.
     * @tparam Table    Underlying table type that owns the column storage.
     * @tparam Ty       Column types to iterate over.
     */
    template<bool IsConst, class Table, class... Ty>
    class basic_table_iterator
    {
    private:
        template<bool, class, class...>
        friend class basic_table_iterator;

    public:
        /** @brief Table type, const-qualified according to @p IsConst. */
        using table_type = std::conditional_t<IsConst, const Table, Table>;

        /** @brief Pointer to table, const-qualified according to @p IsConst. */
        using table_ptr = table_type*;

        /** @brief Satisfies @c std::random_access_iterator_tag. */
        using iterator_category = std::random_access_iterator_tag;

        /** @brief Tuple of unqualified column types. */
        using value_type = std::tuple<std::remove_const_t<Ty>...>;

        /** @brief Signed difference type for pointer arithmetic. */
        using difference_type = ptrdiff_t;

        /** @brief Unsigned index type. */
        using size_type = size_t;

        /** @brief No pointer type — iterator yields tuple references, not addressable values. */
        using pointer = void;

        /** @brief Tuple of (const) lvalue references to each column element at the current position. */
        using reference = std::conditional_t<IsConst, std::tuple<const std::remove_const_t<Ty>&...>, std::tuple<Ty&...>>;

    public:
        /** @brief Default-constructs a singular (invalid) iterator. */
        constexpr basic_table_iterator() noexcept = default;

        /**
         * @brief Constructs an iterator pointing to position @p pos in @p table.
         * @param table Non-null pointer to the owning table.
         * @param pos   Initial column index.
         */
        constexpr basic_table_iterator(table_ptr table, size_type pos) noexcept
            : m_table(table)
            , m_pos(pos)
        {
        }

        /**
         * @brief Implicit conversion from mutable to const iterator.
         * @note Only available when @p IsConst is @c false.
         */
        [[nodiscard]] operator basic_table_iterator<true, Table, Ty...>() const noexcept
            requires(!IsConst)
        {
            return basic_table_iterator<true, Table, Ty...>(m_table, m_pos);
        }

        /**
         * @brief Returns a tuple of references to the element at offset @p value from current position.
         * @param value Signed offset from the current position.
         */
        [[nodiscard]] constexpr reference operator[](difference_type value) const noexcept
        {
            return std::forward_as_tuple(
                m_table->template get<std::remove_const_t<Ty>>()[m_pos + value]...
            );
        }

        /** @brief Dereferences the iterator, returning a tuple of references to the current element. */
        [[nodiscard]] constexpr reference operator*() const noexcept
        {
            return std::forward_as_tuple(
                m_table->template get<std::remove_const_t<Ty>>()[m_pos]...
            );
        }

        /** @brief Pre-increments the iterator. */
        constexpr basic_table_iterator& operator++() noexcept
        {
            ++m_pos;
            return *this;
        }

        /** @brief Post-increments the iterator, returning a copy of the previous state. */
        constexpr basic_table_iterator operator++(int) noexcept
        {
            auto copy = *this;
            ++m_pos;
            return copy;
        }

        /** @brief Advances the iterator by @p value positions. */
        constexpr basic_table_iterator& operator+=(difference_type value) noexcept
        {
            m_pos += value;
            return *this;
        }

        /** @brief Returns a copy of the iterator advanced by @p value positions. */
        constexpr basic_table_iterator operator+(difference_type value) const noexcept
        {
            auto copy = *this;
            return (copy += value);
        }

        /** @brief Pre-decrements the iterator. */
        constexpr basic_table_iterator& operator--() noexcept
        {
            --m_pos;
            return *this;
        }

        /** @brief Post-decrements the iterator, returning a copy of the previous state. */
        constexpr basic_table_iterator operator--(int) noexcept
        {
            auto copy = *this;
            --m_pos;
            return copy;
        }

        /** @brief Moves the iterator back by @p value positions. */
        constexpr basic_table_iterator& operator-=(difference_type value) noexcept
        {
            m_pos -= value;
            return *this;
        }

        /** @brief Returns a copy of the iterator moved back by @p value positions. */
        constexpr basic_table_iterator operator-(difference_type value) const noexcept
        {
            auto copy = *this;
            return (copy -= value);
        }

        /**
         * @brief Returns the signed distance between two iterators.
         * @param other Iterator to subtract from @c *this.
         */
        [[nodiscard]] constexpr difference_type operator-(const basic_table_iterator& other) const noexcept
        {
            return static_cast<difference_type>(m_pos) - static_cast<difference_type>(other.m_pos);
        }

        /** @brief Three-way comparison based on position. */
        [[nodiscard]] constexpr auto operator<=>(const basic_table_iterator& other) const noexcept
        {
            return m_pos <=> other.m_pos;
        }

        /** @brief Equality comparison based on position. */
        [[nodiscard]] constexpr bool operator==(const basic_table_iterator& other) const noexcept
        {
            return m_pos == other.m_pos;
        }

    private:
        /// Pointer to the owning table.
        table_ptr m_table = nullptr;

        /// Current row index within the table.
        size_type m_pos = 0;
    };

}

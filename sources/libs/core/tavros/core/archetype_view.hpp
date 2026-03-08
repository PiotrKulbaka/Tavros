#pragma once

#include <tavros/core/containers/table.hpp>

namespace tavros::core
{

    /**
     * @brief Non-owning view over an archetype's container for a subset of components.
     *
     * Provides iteration, random access, and bulk-traversal over a set of ECS columns
     * stored in a @p Table. All operations are O(1) or O(n); no heap allocation occurs.
     *
     * @tparam Table      Underlying storage type (e.g. basic_archetype).
     * @tparam Components Component types exposed by this view. May be cv-qualified to
     *                    restrict mutability (e.g. @c const Position restricts that column
     *                    to read-only access).
     *
     * @note The view does **not** own the table — the caller must ensure the table
     *       outlives the view.
     */
    template<class Table, class... Components>
        requires(sizeof...(Components) > 0)
    class basic_archetype_view
    {
    public:
        /** @brief Type list of all exposed component types (cvref-stripped). */
        using components = type_list<std::remove_cvref_t<Components>...>;

        /** @brief Unsigned size and index type. */
        using size_type = std::size_t;

        /** @brief Signed difference type. */
        using difference_type = std::ptrdiff_t;

        /**
         * @brief Tuple of lvalue references to each component column at a given row.
         *
         * The reference qualifiers mirror those of @p Components: a @c const-qualified
         * component type yields a @c const lvalue reference.
         */
        using reference = std::tuple<Components&...>;

        /**
         * @brief Tuple of const lvalue references to each component column at a given row.
         */
        using const_reference = std::tuple<const std::remove_const_t<Components>&...>;

        /** @brief Mutable random-access iterator over all columns simultaneously. */
        using iterator = basic_table_iterator<false, Table, Components...>;

        /** @brief Immutable random-access iterator over all columns simultaneously. */
        using const_iterator = basic_table_iterator<true, Table, Components...>;

        /** @brief Mutable reverse iterator. */
        using reverse_iterator = std::reverse_iterator<iterator>;

        /** @brief Immutable reverse iterator. */
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    public:
        /**
         * @brief Constructs a view over @p table.
         * @param table  The underlying storage; must outlive this view.
         */
        basic_archetype_view(Table& table)
            : m_table(std::addressof(table))
        {
        }

        /**
         * @brief Returns a tuple of mutable references to components at @p index.
         * @param index  Row index; must be in [0, size()).
         */
        [[nodiscard]] reference operator[](size_type index) noexcept
        {
            return std::tie(component_at<Components>(index)...);
        }

        /**
         * @brief Returns a tuple of const references to components at @p index.
         * @param index  Row index; must be in [0, size()).
         */
        [[nodiscard]] const_reference operator[](size_type index) const noexcept
        {
            return std::tie(component_at<Components>(index)...);
        }

        /**
         * @brief Returns a mutable reference to component @p T at @p index.
         * @tparam T     Component type; must be one of @p Components.
         * @param index  Row index; must be in [0, size()).
         */
        template<class T>
        [[nodiscard]] T& component_at(size_type index) noexcept
        {
            return m_table->template get<std::remove_cvref_t<T>>()[index];
        }

        /**
         * @brief Returns a const reference to component @p T at @p index.
         * @tparam T     Component type; must be one of @p Components.
         * @param index  Row index; must be in [0, size()).
         */
        template<class T>
        [[nodiscard]] const T& component_at(size_type index) const noexcept
        {
            return m_table->template get<std::remove_cvref_t<T>>()[index];
        }

        /**
         * @brief Calls @p func(Components&...) for every row in [0, size()).
         * @tparam Func  Callable matching @c void(Components&...) or compatible.
         * @param func   Invoked once per row; forwarded on the first call only.
         */
        template<class Func>
        void each(Func&& func)
        {
            const size_type sz = m_table->size();
            for (size_type i = 0; i < sz; ++i) {
                func(component_at<Components>(i)...);
            }
        }

        /** @copydoc each(Func&&) */
        template<class Func>
        void each(Func&& func) const
        {
            const size_type sz = m_table->size();
            for (size_type i = 0; i < sz; ++i) {
                func(component_at<Components>(i)...);
            }
        }

        /**
         * @brief Calls @p func(Components&...) for every row in [@p first, @p first + @p count).
         * @param first  Start index.
         * @param count  Number of rows to visit.
         * @param func   Callable matching @c void(Components&...) or compatible.
         * @pre  <tt>first + count <= size()</tt>
         */
        template<class Func>
        void each_n(size_type first, size_type count, Func&& func)
        {
            TAV_ASSERT(first + count <= m_table->size());
            const size_type end = first + count;
            for (size_type i = first; i < end; ++i) {
                func(component_at<Components>(i)...);
            }
        }

        /** @copydoc each_n(size_type, size_type, Func&&) */
        template<class Func>
        void each_n(size_type first, size_type count, Func&& func) const
        {
            TAV_ASSERT(first + count <= m_table->size());
            const size_type end = first + count;
            for (size_type i = first; i < end; ++i) {
                func(component_at<Components>(i)...);
            }
        }

        /**
         * @brief Calls @p func(size_type index, Components&...) for every row.
         *
         * Identical to @ref each, but also passes the current row index as the
         * first argument — useful when the caller needs to correlate results with
         * entity IDs or perform sparse updates.
         *
         * @tparam Func  Callable matching @c void(size_type, Components&...).
         */
        template<class Func>
        void each_indexed(Func&& func)
        {
            for (size_type i = 0; i < m_table->size(); ++i) {
                func(i, component_at<Components>(i)...);
            }
        }

        /** @copydoc each_indexed(Func&&) */
        template<class Func>
        void each_indexed(Func&& func) const
        {
            for (size_type i = 0; i < m_table->size(); ++i) {
                func(i, component_at<Components>(i)...);
            }
        }

        /**
         * @brief Calls @p func(size_type index, Components&...) for rows in
         *        [@p first, @p first + @p count).
         * @param first  Start index.
         * @param count  Number of rows to visit.
         * @param func   Callable matching @c void(size_type, Components&...).
         * @pre  <tt>first + count <= size()</tt>
         */
        template<class Func>
        void each_n_indexed(size_type first, size_type count, Func&& func)
        {
            TAV_ASSERT(first + count <= m_table->size());
            const size_type end = first + count;
            for (size_type i = first; i < end; ++i) {
                func(i, component_at<Components>(i)...);
            }
        }

        /** @copydoc each_n_indexed(size_type, size_type, Func&&) */
        template<class Func>
        void each_n_indexed(size_type first, size_type count, Func&& func) const
        {
            TAV_ASSERT(first + count <= m_table->size());
            const size_type end = first + count;
            for (size_type i = first; i < end; ++i) {
                func(i, component_at<Components>(i)...);
            }
        }

        /** @brief Returns the number of rows in the view. */
        [[nodiscard]] size_type size() const noexcept
        {
            return m_table->size();
        }

        /** @brief Returns @c true if the view contains no rows. */
        [[nodiscard]] bool empty() const noexcept
        {
            return m_table->empty();
        }

        /** @brief Returns a mutable iterator to the first row. */
        [[nodiscard]] iterator begin() noexcept
        {
            return {m_table, 0};
        }

        /** @brief Returns a mutable iterator past the last row. */
        [[nodiscard]] iterator end() noexcept
        {
            return {m_table, m_table->size()};
        }

        /** @brief Returns an immutable iterator to the first row. */
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return {m_table, 0};
        }

        /** @brief Returns an immutable iterator past the last row. */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return {m_table, m_table->size()};
        }

        /** @brief Returns a const iterator to the first row. */
        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return begin();
        }

        /** @brief Returns a const iterator past the last row. */
        [[nodiscard]] const_iterator cend() const noexcept
        {
            return end();
        }

        /** @brief Returns a mutable reverse iterator to the last row. */
        [[nodiscard]] reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        /** @brief Returns a mutable reverse iterator before the first row. */
        [[nodiscard]] reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        /** @brief Returns a const reverse iterator to the last row. */
        [[nodiscard]] const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        /** @brief Returns a const reverse iterator before the first row. */
        [[nodiscard]] const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        /** @brief Returns a const reverse iterator to the last row. */
        [[nodiscard]] const_reverse_iterator crbegin() const noexcept
        {
            return rbegin();
        }

        /** @brief Returns a const reverse iterator before the first row. */
        [[nodiscard]] const_reverse_iterator crend() const noexcept
        {
            return rend();
        }

    private:
        /// Pointer to the underlying container.
        Table* m_table;
    };

} // namespace tavros::core

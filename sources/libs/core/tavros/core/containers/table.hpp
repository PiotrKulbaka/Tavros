#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/core/containers/table_iterator.hpp>

namespace tavros::core
{

    /**
     * @brief Structure-of-Arrays (SoA) container over a fixed set of typed columns.
     *
     * Stores each column type @p Ty in a separate contiguous vector, providing
     * row-wise access via tuples of references. Satisfies most named requirements
     * of a reversible container. Duplicate column types are allowed at this level -
     * uniqueness constraints belong to higher-level constructs such as archetypes.
     *
     * @tparam Ty Column types. Pack must be non-empty.
     */
    template<class... Ty>
        requires(sizeof...(Ty) > 0) && are_nothrow_default_constructible_v<type_list<Ty...>>
    class basic_table
    {
        template<class T>
        using vector_type = vector<T>;
        using storage_type = std::tuple<vector_type<Ty>...>;

    public:
        /** @brief Type list of all column types as declared. */
        using types = type_list<Ty...>;

        /** @brief Type list of column types with cv-ref qualifiers removed. */
        using unqualified_types = type_transform_t<std::remove_cvref, types>;

        /** @brief Number of columns in this table. */
        static constexpr size_t column_count = list_size_v<types>;

        /** @brief Unsigned size and index type. */
        using size_type = size_t;

        /** @brief Signed difference type. */
        using difference_type = ptrdiff_t;

        /** @brief No single value type - rows are heterogeneous tuples. */
        using value_type = void;

        /** @brief No allocator exposure - allocation is managed internally per column. */
        using allocator_type = void;

        /** @brief Tuple of lvalue references to each column element at a given row. */
        using reference = std::tuple<Ty&...>;

        /** @brief Tuple of const lvalue references to each column element at a given row. */
        using const_reference = std::tuple<const std::remove_const_t<Ty>&...>;

        /** @brief No pointer type - rows are not contiguously addressable as a unit. */
        using pointer = void;

        /** @brief No const pointer type. */
        using const_pointer = void;

        /** @brief Mutable random-access iterator over all columns simultaneously. */
        using iterator = basic_table_iterator<false, basic_table, Ty...>;

        /** @brief Const random-access iterator over all columns simultaneously. */
        using const_iterator = basic_table_iterator<true, basic_table, Ty...>;

        /** @brief Mutable reverse iterator. */
        using reverse_iterator = std::reverse_iterator<iterator>;

        /** @brief Const reverse iterator. */
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    public:
        /** @brief Default-constructs an empty table with no rows. */
        basic_table() noexcept = default;

        /** @brief Move-constructs the table, transferring ownership of all column storage. */
        basic_table(basic_table&& other) noexcept = default;

        /**
         * @brief Move-assigns the table, transferring ownership of all column storage.
         * @return Reference to @c *this.
         */
        basic_table& operator=(basic_table&& other) noexcept = default;

        /** @brief Destroys all column vectors and releases their memory. */
        ~basic_table() noexcept = default;

        /**
         * @brief Returns a tuple of references to all column elements at @p index.
         * @param index Row index. Behavior is undefined if @p index >= size().
         */
        [[nodiscard]] reference operator[](size_type index) noexcept
        {
            return std::forward_as_tuple(get<Ty>()[index]...);
        }

        /** @brief Const overload of @ref operator[]. */
        [[nodiscard]] const_reference operator[](size_type index) const noexcept
        {
            return std::forward_as_tuple(get<Ty>()[index]...);
        }

        /**
         * @brief Returns a tuple of references to the first row.
         * @pre !empty()
         */
        [[nodiscard]] reference front() noexcept
        {
            TAV_ASSERT(!empty());
            return (*this)[static_cast<size_type>(0)];
        }

        /** @brief Const overload of @ref front(). */
        [[nodiscard]] const_reference front() const noexcept
        {
            TAV_ASSERT(!empty());
            return (*this)[static_cast<size_type>(0)];
        }

        /**
         * @brief Returns a tuple of references to the last row.
         * @pre !empty()
         */
        [[nodiscard]] reference back() noexcept
        {
            TAV_ASSERT(!empty());
            return (*this)[static_cast<size_type>(size() - 1)];
        }

        /** @brief Const overload of @ref back(). */
        [[nodiscard]] const_reference back() const noexcept
        {
            TAV_ASSERT(!empty());
            return (*this)[static_cast<size_type>(size() - 1)];
        }

        /**
         * @brief Returns a reference to the column vector at compile-time index @p I.
         * @tparam I Column index. Ill-formed if @p I >= column_count.
         */
        template<size_t I>
            requires(I < column_count)
        [[nodiscard]] auto& get() noexcept
        {
            return std::get<I>(m_storage);
        }

        /** @brief Const overload of index-based @ref get(). */
        template<size_t I>
            requires(I < column_count)
        [[nodiscard]] auto& get() const noexcept
        {
            return std::get<I>(m_storage);
        }

        /**
         * @brief Returns a reference to the column vector for type @p T.
         * @tparam T Column type. Ill-formed if @p T is not among @p Ty.
         */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] auto& get() noexcept
        {
            return std::get<vector_type<T>>(m_storage);
        }

        /** @brief Const overload of type-based @ref get(). */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] const auto& get() const noexcept
        {
            return std::get<vector_type<T>>(m_storage);
        }

        /** @brief Returns true if the table contains no rows. */
        [[nodiscard]] bool empty() const noexcept
        {
            return std::get<0>(m_storage).empty();
        }

        /** @brief Returns the number of rows. */
        [[nodiscard]] size_type size() const noexcept
        {
            return std::get<0>(m_storage).size();
        }

        /** @brief Returns the maximum number of rows the underlying allocator can support. */
        [[nodiscard]] size_type max_size() const noexcept
        {
            return std::get<0>(m_storage).max_size();
        }

        /**
         * @brief Reserves storage for at least @p new_cap rows in all columns.
         * @param new_cap Minimum capacity to reserve.
         */
        void reserve(size_type new_cap)
        {
            (get<Ty>().reserve(new_cap), ...);
        }

        /** @brief Returns the current allocated capacity in rows. */
        [[nodiscard]] size_type capacity() const noexcept
        {
            return std::get<0>(m_storage).capacity();
        }

        /** @brief Releases excess capacity in all column vectors. */
        void shrink_to_fit()
        {
            (get<Ty>().shrink_to_fit(), ...);
        }

        /** @brief Removes all rows from all columns. */
        void clear() noexcept
        {
            (get<Ty>().clear(), ...);
        }

        /**
         * @brief Appends a row by moving positional arguments into each column in declaration order.
         * @param args One value per column, in the same order as @p Ty.
         * @return Reference tuple to the newly inserted row.
         */
        reference emplace_back(std::remove_cvref_t<Ty>&&... args)
        {
            [&]<size_t... Is>(std::index_sequence<Is...>) {
                auto argpack = std::forward_as_tuple(std::move(args)...);
                (std::get<Is>(m_storage).emplace_back(std::move(std::get<Is>(argpack))), ...);
            }(std::make_index_sequence<column_count>{});

            return (*this)[size() - 1];
        }

        /**
         * @brief Appends a row by matching arguments to columns by type.
         *
         * Arguments may be passed in any order. Columns not covered by @p Ts
         * are default-constructed. Requires all passed types to be unique and
         * to be a subset of the table's column types.
         *
         * @tparam Ts Argument types, deduced from @p args.
         * @return Reference tuple to the newly inserted row.
         */
        template<class... Ts>
            requires are_unique_v<type_transform_t<std::remove_cvref, type_list<Ts...>>> && is_subset_v<type_transform_t<std::remove_cvref, type_list<Ts...>>, unqualified_types>
        reference typed_emplace_back(Ts&&... args)
        {
            ([&]<class C>() {
                if constexpr ((std::is_same_v<C, std::remove_cvref_t<Ts>> || ...)) {
                    get<C>().emplace_back(pick_arg<C>(std::forward<Ts>(args)...));
                } else {
                    get<C>().emplace_back();
                }
            }.template operator()<std::remove_cvref_t<Ty>>(),
             ...);

            return (*this)[size() - 1];
        }

        /** @brief Removes the last row from all columns. @pre !empty() */
        void pop_back()
        {
            (get<Ty>().pop_back(), ...);
        }

        /**
         * @brief Removes the row at @p index by swapping it with the last row then popping.
         * @param index Row to remove. Does not preserve row order.
         * @pre index < size()
         */
        void swap_and_pop(size_type index) noexcept
        {
            TAV_ASSERT(index < size());
            if (index >= size()) {
                return;
            }

            size_type last = size() - 1;
            ((get<Ty>()[index] = std::move(get<Ty>()[last])), ...);
            pop_back();
        }

        /**
         * @brief Resizes all columns to @p count rows.
         * @param count New row count. New rows are default-constructed.
         */
        void resize(size_type count)
        {
            (get<Ty>().resize(count), ...);
        }

        /**
         * @brief Swaps the contents of this table with @p other.
         * @note Only available when all column vector types are nothrow-swappable.
         */
        void swap(basic_table& other) noexcept
            requires are_nothrow_swappable_v<type_list<Ty...>>
        {
            (std::swap(get<Ty>(), other.get<Ty>()), ...);
        }

        /** @brief Returns a mutable iterator to the first row. */
        [[nodiscard]] iterator begin() noexcept
        {
            return iterator(this, 0);
        }

        /** @brief Returns a mutable iterator past the last row. */
        [[nodiscard]] iterator end() noexcept
        {
            return iterator(this, size());
        }

        /** @brief Returns a const iterator to the first row. */
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return const_iterator(this, 0);
        }

        /** @brief Returns a const iterator past the last row. */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return const_iterator(this, size());
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
        /**
         * @brief Selects the argument whose unqualified type matches @p C from a forwarded pack.
         * @tparam C Target type to match.
         */
        template<class C, class T, class... Rest>
        static decltype(auto) pick_arg(T&& first, Rest&&... rest)
        {
            if constexpr (std::is_same_v<C, std::remove_cvref_t<T>>) {
                return std::forward<T>(first);
            } else {
                return pick_arg<C>(std::forward<Rest>(rest)...);
            }
        }

    private:
        /// Tuple of per-column vectors.
        storage_type m_storage;
    };

} // namespace tavros::core

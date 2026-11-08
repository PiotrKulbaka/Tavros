#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/traits.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/memory/buffer_view.hpp>

namespace tavros::core
{

    /**
     * @brief Lightweight reference to a single element of type @p T stored in a
     * @ref basic_typed_vectors.
     *
     * Opaque index, not stable across removals from the owning column.
     */
    template<class T>
    struct item_view
    {
        using value_type = T;

        static constexpr uint32 invalid_index = static_cast<uint32>(-1);

        uint32 index = invalid_index;

        [[nodiscard]] constexpr bool valid() const noexcept
        {
            return index != invalid_index;
        }

        [[nodiscard]] friend bool operator==(const item_view&, const item_view&) noexcept = default;
    };

    /**
     * @brief Lightweight reference to a contiguous run of elements of type @p T
     * stored in a @ref basic_typed_vectors.
     *
     * Opaque [first, first + size) range, not stable across removals from the
     * owning column.
     */
    template<class T>
    struct range_view
    {
        using value_type = T;

        static constexpr uint32 invalid_index = static_cast<uint32>(-1);

        uint32 first = invalid_index;
        uint32 size = 0;

        [[nodiscard]] bool valid() const noexcept
        {
            return first != invalid_index;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return size == 0;
        }

        [[nodiscard]] friend bool operator==(const range_view&, const range_view&) noexcept = default;
    };

    /**
     * @brief Set of independent typed columns.
     *
     * Stores each type @p Ts in its own contiguous vector. Unlike @ref basic_table,
     * columns are not row-aligned: inserting into one column does not affect
     * the others. Access and mutation are always scoped to a single column type,
     * addressed either by a single-element @ref item_view or a contiguous
     * @ref range_view.
     *
     * @tparam Ts Column types. Pack must be non-empty. Duplicate types are not
     * allowed - each type identifies exactly one column.
     */
    template<class List>
    class basic_typed_vectors;

    template<class... Ts>
        requires(sizeof...(Ts) > 0) && are_unique_v<type_list<Ts...>>
    class basic_typed_vectors<type_list<Ts...>>
    {
        template<class T>
        using vector_type = vector<T>;

        using storage_type = std::tuple<vector_type<Ts>...>;

    public:
        /** @brief Type list of all column types as declared. */
        using types = type_list<Ts...>;

        /** @brief Number of columns. */
        static constexpr size_t column_count = list_size_v<types>;

        /** @brief Unsigned size type. */
        using size_type = size_t;

    public:
        basic_typed_vectors() noexcept = default;
        basic_typed_vectors(basic_typed_vectors&&) noexcept = default;
        basic_typed_vectors& operator=(basic_typed_vectors&&) noexcept = default;
        ~basic_typed_vectors() noexcept = default;

        /** @brief Returns the underlying vector for column @p T. */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] vector_type<T>& get() noexcept
        {
            return std::get<vector_type<T>>(m_storage);
        }

        /** @brief Const overload of @ref get(). */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] const vector_type<T>& get() const noexcept
        {
            return std::get<vector_type<T>>(m_storage);
        }

        /** @brief Number of elements currently stored in column @p T. */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] size_type size() const noexcept
        {
            return get<T>().size();
        }

        /** @brief Capacity of column @p T. */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] size_type capacity() const noexcept
        {
            return get<T>().capacity();
        }

        /** @brief True if column @p T has no elements. */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] bool empty() const noexcept
        {
            return get<T>().empty();
        }

        /** @brief Pointer to the underlying data of column @p T. */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] T* data() noexcept
        {
            return get<T>().data();
        }

        /** @brief Const pointer to the underlying data of column @p T. */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] const T* data() const noexcept
        {
            return get<T>().data();
        }

        /** @brief Reserves capacity in column @p T. */
        template<class T>
            requires contains_type_v<T, types>
        void reserve(size_type new_cap)
        {
            get<T>().reserve(new_cap);
        }

        /** @brief Requests the removal of unused capacity from column @p T. */
        template<class T>
            requires contains_type_v<T, types>
        void shrink_to_fit()
        {
            get<T>().shrink_to_fit();
        }

        /** @brief Requests the removal of unused capacity from every column. */
        void shrink_to_fit_all()
        {
            (get<Ts>().shrink_to_fit(), ...);
        }

        /** @brief Removes all elements from column @p T. Views into it become dangling. */
        template<class T>
            requires contains_type_v<T, types>
        void clear() noexcept
        {
            get<T>().clear();
        }

        /** @brief Removes all elements from every column. All views become dangling. */
        void clear_all() noexcept
        {
            (get<Ts>().clear(), ...);
        }

        /**
         * @brief Appends a single element to column @p T, constructed in place.
         * @return A view referring to the newly inserted element.
         */
        template<class T, class... Args>
            requires contains_type_v<T, types> && std::constructible_from<T, Args...>
        item_view<T> emplace_back(Args&&... args)
        {
            auto& vec = get<T>();
            vec.emplace_back(std::forward<Args>(args)...);
            return item_view<T>{static_cast<uint32>(vec.size() - 1)};
        }

        /**
         * @brief Appends a single element to column @p T by copy.
         * @return A view referring to the newly inserted element.
         */
        template<class T>
            requires contains_type_v<T, types>
        item_view<T> add(const T& value)
        {
            return emplace_back<T>(value);
        }

        /**
         * @brief Appends a single element to column @p T by move.
         * @return A view referring to the newly inserted element.
         */
        template<class T>
            requires contains_type_v<T, types>
        item_view<T> add(T&& value)
        {
            return emplace_back<T>(std::move(value));
        }

        /**
         * @brief Appends a contiguous run of elements to column @p T.
         * @return A view referring to the newly inserted range.
         */
        template<class T>
            requires contains_type_v<T, types>
        range_view<T> add_range(buffer_view<T> items)
        {
            auto&  vec = get<T>();
            uint32 first = static_cast<uint32>(vec.size());
            vec.insert(vec.end(), items.begin(), items.end());
            return range_view<T>{first, static_cast<uint32>(items.size())};
        }

        /** @brief Overload accepting a braced initializer list. */
        template<class T>
            requires contains_type_v<T, types>
        range_view<T> add_range(std::initializer_list<T> items)
        {
            return add_range<T>(buffer_view<T>(items.begin(), items.size()));
        }

        /** @brief Element access for column @p T. @pre v.index < size<T>() */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] T& operator[](item_view<T> v) noexcept
        {
            TAV_ASSERT(v.valid());
            TAV_ASSERT(v.index < size<T>());
            return get<T>()[v.index];
        }

        /** @brief Const overload. */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] const T& operator[](item_view<T> v) const noexcept
        {
            TAV_ASSERT(v.valid());
            TAV_ASSERT(v.index < size<T>());
            return get<T>()[v.index];
        }

        /** @brief Range access for column @p T. @pre v.first + v.size <= size<T>() */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] buffer_view<T> operator[](range_view<T> v) noexcept
        {
            TAV_ASSERT(v.valid());
            TAV_ASSERT(v.first + v.size <= size<T>());
            return buffer_view<T>(get<T>().data() + v.first, v.size);
        }

        /** @brief Const overload. */
        template<class T>
            requires contains_type_v<T, types>
        [[nodiscard]] buffer_view<const T> operator[](range_view<T> v) const noexcept
        {
            TAV_ASSERT(v.valid());
            TAV_ASSERT(v.first + v.size <= size<T>());
            return buffer_view<const T>(get<T>().data() + v.first, v.size);
        }

    private:
        storage_type m_storage;
    };

} // namespace tavros::core
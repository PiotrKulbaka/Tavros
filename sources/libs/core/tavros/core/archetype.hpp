#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/traits.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/containers/table_iterator.hpp>

#include <tuple>

namespace tavros::core
{

    /**
     * @brief Concept: archetype contains all required components.
     */
    template<class ArchetypeT, class... RequiredComponents>
    concept ArchetypeWith = requires {
        typename ArchetypeT::is_archetype;
    } && (is_subset_v<type_list<RequiredComponents...>, class ArchetypeT::unqualified_types>);


    template<class... Ty>
    concept is_all_default_constructible = (std::is_default_constructible_v<Ty> && ...);

    /**
     * @brief Non-owning view over an archetype's container for a subset of components.
     *
     * @tparam Container  Underlying tightly_packed_archetype_base (or compatible) type.
     * @tparam Components Component types exposed by this view.
     */
    template<class Table, class... Components>
    class archetype_view
    {
    public:
        using iterator = basic_table_iterator<false, Table, Components...>;
        using const_iterator = basic_table_iterator<true, Table, Components...>;

    public:
        archetype_view(Table& table)
            : m_table(table)
        {
        }

        /**
         * @brief Returns a tuple of references to all view components at index.
         */
        [[nodiscard]] auto operator[](size_t index) noexcept
        {
            return std::forward_as_tuple(get_component_at<Components>(index)...);
        }

        /**
         * @brief Returns a tuple of const references to all view components at index.
         */
        [[nodiscard]] auto operator[](size_t index) const noexcept
        {
            return std::forward_as_tuple(get_component_at<Components>(index)...);
        }

        /**
         * @brief Get a specific component at given index.
         * @tparam T Component type (must be in Components...).
         */
        template<class T>
        [[nodiscard]] T& get_component_at(size_t index)
        {
            static_assert(
                (std::is_same_v<T, Components> || ...),
                "The passed component type is not contained in the tuple"
            );
            return m_table.template get<std::remove_const_t<T>>()[index];
        }

        /**
         * @brief Get a specific component at given index.
         * @tparam T Component type (must be in Components...).
         */
        template<class T>
        [[nodiscard]] const T& get_component_at(size_t index) const
        {
            static_assert(
                (std::is_same_v<T, Components> || ...),
                "The passed component type is not contained in the tuple"
            );
            return m_table.template get<std::remove_const_t<T>>()[index];
        }

        /**
         * @brief Invoke a callable with all view components at a specific index.
         */
        template<class Func>
        void invoke_at(size_t index, Func&& func)
        {
            func(get_component_at<Components>(index)...);
        }

        /**
         * @brief Invoke a callable with all view components at a specific index.
         */
        template<class Func>
        void invoke_at(size_t index, Func&& func) const
        {
            func(get_component_at<Components>(index)...);
        }

        /**
         * @brief Iterate over all elements, invoking func(Components&...) for each.
         * @note  func is stored by ref — safe to call in a loop.
         */
        template<class Func>
        void each(Func&& func)
        {
            const size_t n = m_table.size();
            for (size_t i = 0; i < n; ++i) {
                func(get_component_at<Components>(i)...);
            }
        }

        /**
         * @brief Iterate over all elements, invoking func(Components&...) for each.
         * @note  func is stored by ref — safe to call in a loop.
         */
        template<class Func>
        void each(Func&& func) const
        {
            const size_t n = m_table.size();
            for (size_t i = 0; i < n; ++i) {
                func(get_component_at<Components>(i)...);
            }
        }

        /**
         * @brief Iterate over [first, first + count) invoking func for each element.
         * @pre   first + count <= size()
         */
        template<class Func>
        void each_n(size_t first, size_t count, Func&& func)
        {
            TAV_ASSERT(first + count <= m_table.size());
            const size_t end = first + count;
            for (size_t i = first; i < end; ++i) {
                func(get_component_at<Components>(i)...);
            }
        }

        /**
         * @brief Iterate over all elements, invoking func(size_t index, Components&...) for each.
         */
        template<class Func>
        void each_indexed(Func&& func)
        {
            const size_t n = m_table.size();
            for (size_t i = 0; i < n; ++i) {
                func(i, get_component_at<Components>(i)...);
            }
        }

        template<class Func>
        void each_indexed(Func&& func) const
        {
            const size_t n = m_table.size();
            for (size_t i = 0; i < n; ++i) {
                func(i, get_component_at<Components>(i)...);
            }
        }

        template<class Func>
        void each_n_indexed(size_t first, size_t count, Func&& func)
        {
            TAV_ASSERT(first + count <= m_table.size());
            const size_t end = first + count;
            for (size_t i = first; i < end; ++i) {
                func(i, get_component_at<Components>(i)...);
            }
        }

        [[nodiscard]] size_t size() const noexcept
        {
            return m_table.size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return m_table.empty();
        }

        [[nodiscard]] iterator begin() noexcept
        {
            return {&m_table, 0};
        }

        [[nodiscard]] iterator end() noexcept
        {
            return {&m_table, m_table.size()};
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return {&m_table, 0};
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return {&m_table, m_table.size()};
        }

    private:
        Table& m_table; /// Reference to the underlying container.
    };


    template<class... Ty>
        requires is_all_default_constructible<Ty...>
    class basic_dense_table : noncopyable
    {
        template<class T>
        using vector_type = vector<T>;
        using storage_type = std::tuple<vector_type<Ty>...>;

    public:
        using types = type_list<Ty...>;
        using unqualified_types = type_transform_t<std::remove_cvref, types>;

        using size_type = size_t;
        using difference_type = ptrdiff_t;

        using value_type = void;
        using allocator_type = void;
        using reference = std::tuple<Ty&...>;
        using const_reference = std::tuple<const std::remove_const_t<Ty>&...>;
        using pointer = void;
        using const_pointer = void;
        using iterator = basic_table_iterator<false, basic_dense_table, Ty...>;
        using const_iterator = basic_table_iterator<true, basic_dense_table, Ty...>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    public:
        [[nodiscard]] reference operator[](size_type index) noexcept
        {
            return std::forward_as_tuple(get<std::remove_const_t<Ty>>()[index]...);
        }

        [[nodiscard]] const_reference operator[](size_type index) const noexcept
        {
            return std::forward_as_tuple(get<std::remove_const_t<Ty>>()[index]...);
        }

        [[nodiscard]] reference front() noexcept
        {
            return this->operator[](static_cast<size_type>(0));
        }

        [[nodiscard]] const_reference front() const noexcept
        {
            return this->operator[](static_cast<size_type>(0));
        }

        [[nodiscard]] reference back() noexcept
        {
            return this->operator[](static_cast<size_type>(size() - 1));
        }

        [[nodiscard]] const_reference back() const noexcept
        {
            return this->operator[](static_cast<size_type>(size() - 1));
        }

        template<class T>
        [[nodiscard]] auto& get() noexcept
        {
            static_assert((std::is_same_v<T, Ty> || ...), "The passed type is not contained in the table");
            return std::get<vector<T>>(m_storage);
        }

        template<class T>
        [[nodiscard]] const auto& get() const noexcept
        {
            static_assert((std::is_same_v<T, Ty> || ...), "The passed type is not contained in the table");
            return std::get<vector<T>>(m_storage);
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return std::get<0>(m_storage).empty();
        }

        [[nodiscard]] size_type size() const noexcept
        {
            return std::get<0>(m_storage).size();
        }

        [[nodiscard]] size_type max_size() const noexcept
        {
            return std::get<0>(m_storage).size();
        }

        void reserve(size_type new_cap)
        {
            (get<Ty>().reserve(new_cap), ...);
        }

        [[nodiscard]] size_type capacity() const noexcept
        {
            return std::get<0>(m_storage).capacity();
        }

        void shrink_to_fit()
        {
            (get<Ty>().shrink_to_fit(), ...);
        }

        void clear() noexcept
        {
            (get<Ty>().clear(), ...);
        }

        template<class... Ts>
        reference emplace_back(Ts&&... args)
        {
            using unqualified_ts = type_transform_t<std::remove_cvref, type_list<Ts...>>;
            static_assert(are_unique_v<unqualified_ts>, "Duplicate component types passed to emplace_back");
            static_assert(is_subset_v<unqualified_ts, unqualified_types>, "One or more passed types are not part of this basic_table");

            ([&]<class C>() {
                if constexpr ((std::is_same_v<C, std::remove_cvref_t<Ts>> || ...)) {
                    get<C>().emplace_back(pick_arg<C>(std::forward<Ts>(args)...));
                } else {
                    get<C>().emplace_back();
                }
            }.template operator()<std::remove_cvref_t<Ty>>(),
             ...);

            auto idx = size() - 1;
            return std::forward_as_tuple(get<std::remove_cvref_t<Ty>>()[idx]...);
        }

        void pop_back()
        {
            (get<Ty>().pop_back(), ...);
        }

        void swap_and_pop(size_type index)
        {
            TAV_ASSERT(index < size());
            if (index >= size()) {
                return;
            }

            size_type last = size() - 1;
            if (last != index) {
                (std::swap(get<Ty>()[index], get<Ty>()[last]), ...);
            }
            pop_back();
        }

        void resize(size_type count)
        {
            (get<Ty>().resize(count), ...);
        }

        void swap(basic_dense_table& other) noexcept
        {
            (std::swap(get<Ty>(), other.get<Ty>()), ...);
        }

        [[nodiscard]] iterator begin() noexcept
        {
            return iterator(this, 0);
        }

        [[nodiscard]] iterator end() noexcept
        {
            return iterator(this, size());
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return const_iterator(this, 0);
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return const_iterator(this, size());
        }

        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return begin();
        }

        [[nodiscard]] const_iterator cend() const noexcept
        {
            return end();
        }

    private:
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
        storage_type m_storage;
    };


    /**
     * @brief SoA (Structure of Arrays) storage for a fixed set of component types.
     *
     * Each component type is stored in its own contiguous vector, enabling
     * cache-friendly iteration. Entities are identified by their index.
     *
     * @tparam Components Component types. Must all be distinct.
     */
    template<class... Components>
        requires are_unique_unqualified_v<type_list<Components...>>
    class tightly_packed_archetype_base final : public basic_dense_table<Components...>
    {
        using base = basic_dense_table<Components...>;

    public:
        /// Marker type to identify this class as an archetype.
        using is_archetype = void;

        /**
         * @brief Create a mutable view over a subset of components.
         *
         * @tparam Ts Components exposed by the view. Must be a subset of Components...
         * @code
         *   auto v = arch.view<Position, Velocity>();
         *   for (auto [pos, vel] : v) { ... }
         * @endcode
         */
        template<class... Ts>
        [[nodiscard]] auto view()
        {
            using unqualified_ts = type_transform_t<std::remove_cvref, type_list<Ts...>>;
            static_assert(is_subset_v<unqualified_ts, typename base::unqualified_types>, "One or more view components are not part of this archetype");
            return archetype_view<tightly_packed_archetype_base, Ts...>(*this);
        }

        template<class... Ts>
        [[nodiscard]] auto view() const
        {
            using unqualified_ts = type_transform_t<std::remove_cvref, type_list<Ts...>>;
            static_assert(is_subset_v<unqualified_ts, typename base::unqualified_types>, "One or more view components are not part of this archetype");
            return archetype_view<const tightly_packed_archetype_base, Ts...>(*this);
        }
    };

} // namespace tavros::core

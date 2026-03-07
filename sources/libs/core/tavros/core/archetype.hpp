#pragma once

#include <tavros/core/containers/table.hpp>

namespace tavros::core
{

    /**
     * @brief Concept: archetype contains all required components.
     */
    template<class ArchetypeT, class... RequiredComponents>
    concept ArchetypeWith = requires {
        typename ArchetypeT::is_archetype;
    } && (is_subset_v<type_list<RequiredComponents...>, class ArchetypeT::unqualified_types>);

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
    class tightly_packed_archetype_base final : public basic_table<Components...>
    {
        using base = basic_table<Components...>;

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

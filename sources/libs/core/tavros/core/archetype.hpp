#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/containers/vector.hpp>

#include <tuple>

namespace tavros::core
{

    /**
     * @brief Checks if a pack of types contains only unique types.
     *
     * @tparam ... Template parameter pack.
     * @note Specialization recursively checks each type against the rest of the pack.
     */
    template<class...>
    struct unique_types
        : std::true_type
    {
    };

    /**
     * @brief Partial specialization for checking uniqueness in a non-empty pack.
     *
     * @tparam T First type in the pack.
     * @tparam Rest Remaining types in the pack.
     */
    template<class T, class... Rest>
    struct unique_types<T, Rest...>
        : std::conditional_t<(std::is_same_v<T, Rest> || ...), std::false_type, unique_types<Rest...>>
    {
    };

    /**
     * @brief Checks if a type exists in a type tuple.
     *
     * @tparam T Type to search for.
     * @tparam Ts Types contained in the tuple.
     */
    template<class T, class... Ts>
    struct contains_type;

    /**
     * @brief Specialization of contains_type for std::tuple.
     *
     * @tparam T Type to search for.
     * @tparam Ts Types contained in the tuple.
     */
    template<class T, class... Ts>
    struct contains_type<T, std::tuple<Ts...>>
        : std::disjunction<std::is_same<T, Ts>...>
    {
    };

    /**
     * @brief Convenience variable template to check type presence in a tuple.
     *
     * @tparam T Type to search for.
     * @tparam Tuple Tuple of types to check.
     */
    template<class T, class Tuple>
    inline constexpr bool contains_type_v = contains_type<T, Tuple>::value;

    /**
     * @brief Concept to check if an archetype contains all required components.
     *
     * @tparam ArchetypeT Archetype type.
     * @tparam RequiredComponents Components required in the archetype.
     */
    template<class ArchetypeT, class... RequiredComponents>
    concept ArchetypeWith = requires {
        typename ArchetypeT::is_archetype;
    } && (contains_type_v<RequiredComponents, class ArchetypeT::components> && ...);


    /**
     * @brief Provides a view over an archetype's container, allowing iteration over specified components.
     *
     * @tparam Container Container type storing components.
     * @tparam Components Component types to access via the view.
     */
    template<class Container, class... Components>
    class archetype_view
    {
    public:
        /**
         * @brief Iterator for archetype_view.
         */
        struct iterator
        {
            Container* m_container; /// Pointer to the container.
            size_t     m_index;     /// Current index.

            iterator& operator++() noexcept
            {
                ++m_index;
                return *this;
            }

            bool operator!=(const iterator& other) const
            {
                return m_index != other.m_index;
            }

            /**
             * @brief Access component of type C at current index.
             * @tparam C Component type.
             * @return Reference to component.
             */
            template<class C>
            decltype(auto) get_component() const
            {
                return static_cast<C&>(m_container->template get<std::remove_const_t<C>>()[m_index]);
            }

            /**
             * @brief Dereference iterator to get a tuple of all components.
             * @return Tuple of references to components.
             */
            auto operator*() const
            {
                return std::forward_as_tuple(get_component<Components>()...);
            }
        };

    public:
        archetype_view(Container& container)
            : m_container(container)
        {
        }

        auto operator[](size_t index) noexcept
        {
            return std::forward_as_tuple(get_component_at<Components>(index)...);
        }

        const auto operator[](size_t index) const noexcept
        {
            return std::forward_as_tuple(get_component_at<Components>(index)...);
        }

        /**
         * @brief Get a specific component at given index.
         *
         * @tparam C Component type.
         * @param index Index of the element.
         * @return Reference to component.
         */
        template<class C>
        decltype(auto) get_component_at(size_t index) const
        {
            static_assert(
                (std::is_same_v<C, Components> || ...),
                "The passed component type is not contained in the tuple"
            );
            return static_cast<C&>(m_container.template get<std::remove_const_t<C>>()[index]);
        }

        /**
         * @brief Invoke a function on the components at a specific index.
         *
         * @tparam Func Callable type.
         * @param index Index of the element.
         * @param func Callable to invoke with components.
         */
        template<class Func>
        void invoke_at(size_t index, Func&& func)
        {
            std::forward<Func>(func)(get_component_at<Components>(index)...);
        }

        /**
         * @brief Iterate over all elements, invoking a callable for each.
         *
         * @tparam Func Callable type.
         * @param func Callable to invoke with components.
         */
        template<class Func>
        void each(Func&& func)
        {
            const size_t n = m_container.size();
            for (size_t i = 0; i < n; ++i) {
                std::forward<Func>(func)(get_component_at<Components>(i)...);
            }
        }

        template<class Func>
        void each_n(size_t first, size_t count, Func&& func)
        {
            size_t end = first + count;
            for (size_t i = first; i < end; ++i) {
                std::forward<Func>(func)(get_component_at<Components>(i)...);
            }
        }

        /**
         * @brief Iterate over all elements with index, invoking a callable for each.
         *
         * @tparam Func Callable type.
         * @param func Callable to invoke with index and components.
         */
        template<class Func>
        void each_indexed(Func&& func)
        {
            const size_t n = m_container.size();
            for (size_t i = 0; i < n; ++i) {
                std::forward<Func>(func)(i, get_component_at<Components>(i)...);
            }
        }

        template<class Func>
        void each_n_indexed(size_t first, size_t count, Func&& func)
        {
            size_t end = first + count;
            for (size_t i = first; i < end; ++i) {
                std::forward<Func>(func)(i, get_component_at<Components>(i)...);
            }
        }

        iterator begin()
        {
            return {&m_container, 0};
        }

        iterator end()
        {
            return {&m_container, m_container.size()};
        }

    private:
        Container& m_container; /// Reference to the underlying container.
    };


    /**
     * @brief Base archetype class for ECS-style storage of components.
     *
     * Stores multiple vectors of component types and provides efficient access, iteration,
     * and modification operations for entities in an archetype.
     *
     * @tparam Components Component types stored in this archetype. Must be unique.
     */
    template<class... Components>
        requires unique_types<Components...>::value
    class archetype_base
    {
    public:
        /// Marker type to identify this class as an archetype.
        using is_archetype = void;

        /// Tuple containing all component types stored in this archetype.
        using components = std::tuple<Components...>;

    public:
        /**
         * @brief Reserve memory for all component vectors.
         *
         * @param new_capacity Number of elements to reserve space for.
         * @note This does not change the logical size of the archetype, only preallocates memory.
         */
        void reserve(size_t new_capacity)
        {
            (get<Components>().reserve(new_capacity), ...);
        }

        /**
         * @brief Resize all component vectors to the given size.
         *
         * @param count New size of each component vector.
         * @note If the new size is larger, default-constructed elements are added.
         */
        void resize(size_t count)
        {
            (get<Components>().resize(count), ...);
        }

        /**
         * @brief Clear all component vectors.
         *
         * After this call, size() will return 0.
         */
        void clear() noexcept
        {
            (get<Components>().clear(), ...);
        }

        /**
         * @brief Get the number of entities stored in this archetype.
         *
         * @return Number of elements (entities) in the archetype.
         */
        size_t size() const noexcept
        {
            return std::get<0>(m_storage).size();
        }

        /**
         * @brief Get the capacity of the archetype (capacity of component vectors).
         *
         * @return Capacity of the archetype.
         */
        size_t capacity() const noexcept
        {
            return std::get<0>(m_storage).capacity();
        }

        /**
         * @brief Check if the archetype contains no entities.
         *
         * @return true if empty, false otherwise.
         */
        bool empty() const noexcept
        {
            return std::get<0>(m_storage).empty();
        }

        /**
         * @brief Access the vector of a specific component type.
         *
         * @tparam T Component type.
         * @return Reference to the vector storing components of type T.
         * @note Static assertion fails if T is not part of the archetype.
         */
        template<class T>
        auto& get() noexcept
        {
            static_assert(
                (std::is_same_v<T, Components> || ...),
                "The passed component type is not contained in the tuple"
            );
            return std::get<vector<T>>(m_storage);
        }

        /**
         * @brief Const access to the vector of a specific component type.
         *
         * @tparam T Component type.
         * @return Const reference to the vector storing components of type T.
         * @note Static assertion fails if T is not part of the archetype.
         */
        template<class T>
        const auto& get() const noexcept
        {
            static_assert(
                (std::is_same_v<T, Components> || ...),
                "The passed component type is not contained in the tuple"
            );
            return std::get<vector<T>>(m_storage);
        }

        /**
         * @brief Add a new entity with given component values.
         *
         * @param ts Component values to emplace.
         * @note Each component is forwarded to its respective vector.
         */
        /*void emplace_back(Components&&... ts)
        {
            (get<Components>().emplace_back(std::forward<Components>(ts)), ...);
        }*/

        template<class C>
        C&& select_arg(C&& arg)
        {
            return std::forward<C>(arg);
        }

        template<class C, class T, class... Rest>
        decltype(auto) select_arg(T&& first, Rest&&... rest)
        {
            if constexpr (std::is_same_v<C, std::remove_cvref_t<T>>) {
                return std::forward<T>(first);
            } else {
                return select_arg<C>(std::forward<Rest>(rest)...);
            }
        }

        template<class C, class... Ts>
        decltype(auto) get_arg_or_default(Ts&&... args)
        {
            if constexpr ((std::is_same_v<C, std::remove_cvref_t<Ts>> || ...)) {
                return select_arg<C>(std::forward<Ts>(args)...);
            } else {
                return C{};
            }
        }

        template<class... Ts>
        void emplace_back(Ts&&... args)
        {
            static_assert(unique_types<std::remove_cvref_t<Ts>...>::value, "Duplicate component types passed");

            static_assert((contains_type_v<std::remove_reference_t<Ts>, components> && ...), "One of the types is not in the archetype");

            ([&] {
                using C = Components;

                if constexpr ((std::is_same_v<C, std::remove_cvref_t<Ts>> || ...)) {
                    get<C>().emplace_back(
                        get_arg_or_default<C>(std::forward<Ts>(args)...)
                    );
                } else {
                    get<C>().emplace_back();
                }
            }(),
             ...);
        }

        /**
         * @brief Remove an entity at the given index using swap-and-pop.
         *
         * @param index Index of the entity to remove.
         * @note Swaps the last element into the removed position and pops the last element
         *       from all component vectors. This is O(1) but does not preserve order.
         */
        void swap_erase(size_t index)
        {
            const size_t last = size() - 1;

            (std::swap(get<Components>(m_storage)[index], get<Components>(m_storage)[last]), ...);
            (std::get<vector<Components>>(m_storage).pop_back(), ...);
        }

        /**
         * @brief Access raw pointer to the array of a specific component type.
         *
         * @tparam T Component type.
         * @return Pointer to the underlying array of components.
         */
        template<class T>
        T* data()
        {
            return get<T>().data();
        }

        /**
         * @brief Const access to raw pointer of a specific component type.
         *
         * @tparam T Component type.
         * @return Const pointer to the underlying array of components.
         */
        template<class T>
        const T* data() const
        {
            return get<T>().data();
        }

        /**
         * @brief Create a view over this archetype for specific components.
         *
         * @tparam ViewComponents Components to include in the view.
         * @return archetype_view providing iteration over the specified components.
         */
        template<class... ViewComponents>
        auto view()
        {
            return archetype_view<archetype_base, ViewComponents...>(*this);
        }

    private:
        std::tuple<vector<Components>...> m_storage;
    };

} // namespace tavros::core

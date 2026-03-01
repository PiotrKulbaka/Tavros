#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/containers/vector.hpp>

#include <tuple>

namespace tavros::core
{

    /**
     * @brief Checks if a pack of types contains only unique types.
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
     * @brief Concept: archetype contains all required components.
     */
    template<class ArchetypeT, class... RequiredComponents>
    concept ArchetypeWith = requires {
        typename ArchetypeT::is_archetype;
    } && (contains_type_v<RequiredComponents, class ArchetypeT::components> && ...);


    /**
     * @brief Non-owning view over an archetype's container for a subset of components.
     *
     * @tparam Container  Underlying archetype_base (or compatible) type.
     * @tparam Components Component types exposed by this view.
     */
    template<class Container, class... Components>
    class archetype_view
    {
    public:
        template<bool IsConst>
        class iterator_base
        {
        public:
            using container_ptr = std::conditional_t<IsConst, const Container*, Container*>;
            using iterator_category = std::random_access_iterator_tag;
            using value_type = std::conditional_t<IsConst, std::tuple<const Components&...>, std::tuple<Components&...>>;
            using difference_type = std::ptrdiff_t;
            using pointer = void;
            using reference = value_type;

        public:
            template<bool OtherConst>
                requires(IsConst && !OtherConst)
            iterator_base(const iterator_base<OtherConst>& other) noexcept
                : m_container(other.m_container)
                , m_index(other.m_index)
            {
            }

            iterator_base& operator++() noexcept
            {
                ++m_index;
                return *this;
            }

            iterator_base operator++(int) noexcept
            {
                iterator_base tmp = *this;
                ++m_index;
                return tmp;
            }

            iterator_base& operator--() noexcept
            {
                --m_index;
                return *this;
            }

            iterator_base operator--(int) noexcept
            {
                iterator_base tmp = *this;
                --m_index;
                return tmp;
            }

            iterator_base& operator+=(difference_type n) noexcept
            {
                m_index = static_cast<size_t>(static_cast<difference_type>(m_index) + n);
                return *this;
            }

            iterator_base& operator-=(difference_type n) noexcept
            {
                return (*this) += -n;
            }

            iterator_base operator+(difference_type n) const noexcept
            {
                iterator_base tmp = *this;
                return tmp += n;
            }

            iterator_base operator-(difference_type n) const noexcept
            {
                iterator_base tmp = *this;
                return tmp -= n;
            }

            difference_type operator-(const iterator_base& other) const noexcept
            {
                return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index);
            }

            bool operator<=>(const iterator_base& other) const noexcept
            {
                return m_index <=> other.m_index;
            }

            /**
             * @brief Returns a tuple of (const) references to all view components at current index.
             */
            reference operator*() const
            {
                return std::forward_as_tuple(
                    m_container->template get<std::remove_const_t<Components>>()[m_index]...
                );
            }

            reference operator[](difference_type n) const
            {
                return *(*this + n);
            }

            template<class C>
            decltype(auto) get_component() const
            {
                static_assert(
                    (std::is_same_v<std::remove_const_t<C>, Components> || ...),
                    "Component type is not part of this view"
                );
                using result_t = std::conditional_t<IsConst, const std::remove_const_t<C>, std::remove_const_t<C>>;
                return static_cast<result_t&>(
                    m_container->template get<std::remove_const_t<C>>()[m_index]
                );
            }

            friend iterator_base operator+(difference_type n, const iterator_base& it) noexcept
            {
                return it + n;
            }

        private:
            container_ptr m_container;
            size_t        m_index;
        };

        using iterator = iterator_base<false>;
        using const_iterator = iterator_base<true>;

    public:
        archetype_view(Container& container)
            : m_container(container)
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
            return m_container.template get<std::remove_const_t<T>>()[index];
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
            return m_container.template get<std::remove_const_t<T>>()[index];
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
            const size_t n = m_container.size();
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
            const size_t n = m_container.size();
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
            TAV_ASSERT(first + count <= m_container.size());
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
            const size_t n = m_container.size();
            for (size_t i = 0; i < n; ++i) {
                func(i, get_component_at<Components>(i)...);
            }
        }

        template<class Func>
        void each_indexed(Func&& func) const
        {
            const size_t n = m_container.size();
            for (size_t i = 0; i < n; ++i) {
                func(i, get_component_at<Components>(i)...);
            }
        }

        template<class Func>
        void each_n_indexed(size_t first, size_t count, Func&& func)
        {
            TAV_ASSERT(first + count <= m_container.size());
            const size_t end = first + count;
            for (size_t i = first; i < end; ++i) {
                func(i, get_component_at<Components>(i)...);
            }
        }

        [[nodiscard]] size_t size() const noexcept
        {
            return m_container.size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return m_container.empty();
        }

        [[nodiscard]] iterator begin() noexcept
        {
            return {&m_container, 0};
        }

        [[nodiscard]] iterator end() noexcept
        {
            return {&m_container, m_container.size()};
        }

        [[nodiscard]] const_iterator begin() const noexcept
        {
            return {&m_container, 0};
        }

        [[nodiscard]] const_iterator end() const noexcept
        {
            return {&m_container, m_container.size()};
        }

    private:
        Container& m_container; /// Reference to the underlying container.
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
         * @brief Release excess capacity.
         */
        void shrink_to_fit()
        {
            (get<Components>().shrink_to_fit(), ...);
        }

        /**
         * @brief Get the number of entities stored in this archetype.
         *
         * @return Number of elements (entities) in the archetype.
         */
        [[nodiscard]] size_t size() const noexcept
        {
            return std::get<0>(m_storage).size();
        }

        /**
         * @brief Get the capacity of the archetype (capacity of component vectors).
         *
         * @return Capacity of the archetype.
         */
        [[nodiscard]] size_t capacity() const noexcept
        {
            return std::get<0>(m_storage).capacity();
        }

        /**
         * @brief Check if the archetype contains no entities.
         *
         * @return true if empty, false otherwise.
         */
        [[nodiscard]] bool empty() const noexcept
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
        [[nodiscard]] auto& get() noexcept
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
        [[nodiscard]] const auto& get() const noexcept
        {
            static_assert(
                (std::is_same_v<T, Components> || ...),
                "The passed component type is not contained in the tuple"
            );
            return std::get<vector<T>>(m_storage);
        }

        /**
         * @brief Access raw pointer to the array of a specific component type.
         *
         * @tparam T Component type.
         * @return Pointer to the underlying array of components.
         */
        template<class T>
        [[nodiscard]] T* data()
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
        [[nodiscard]] const T* data() const
        {
            return get<T>().data();
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

        /**
         * @brief Append a new entity, passing components in any order.
         *
         * Provided component types must be a subset of Components...
         * (no duplicates allowed). Omitted components are default-constructed.
         *
         * @code
         *   arch.emplace_back(Position{1,2}, Velocity{3,4});
         *   arch.emplace_back(Velocity{3,4}, Position{1,2}); // same result
         * @endcode
         */
        template<class... Ts>
        void emplace_back(Ts&&... args)
        {
            static_assert(unique_types<std::remove_cvref_t<Ts>...>::value, "Duplicate component types passed to emplace_back");
            static_assert((contains_type_v<std::remove_cvref_t<Ts>, components> && ...), "One or more passed types are not part of this archetype");

            ([&]<class C>() {
                if constexpr ((std::is_same_v<C, std::remove_cvref_t<Ts>> || ...)) {
                    get<C>().emplace_back(pick_arg<C>(std::forward<Ts>(args)...));
                } else {
                    get<C>().emplace_back();
                }
            }.template operator()<Components>(),
             ...);
        }

        /**
         * @brief Remove an entity at the given index using swap-and-pop.
         *
         * @param index Index of the entity to remove.
         * @note Swaps the last element into the removed position and pops the last element
         *       from all component vectors. This is O(1) but does not preserve order.
         */
        size_t swap_erase(size_t index)
        {
            TAV_ASSERT(index < size());
            const size_t last = size() - 1;
            if (index != last) {
                (std::swap(get<Components>()[index], get<Components>()[last]), ...);
            }

            (get<Components>().pop_back(), ...);
            return last;
        }

        /**
         * @brief Swap the contents of two archetypes.
         */
        void swap(archetype_base& other) noexcept
        {
            (std::swap(get<Components>(), other.get<Components>()), ...);
        }

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
            static_assert((contains_type_v<std::remove_cvref_t<Ts>, components> && ...), "One or more view components are not part of this archetype");
            return archetype_view<archetype_base, Ts...>(*this);
        }

        template<class... Ts>
        [[nodiscard]] auto view() const
        {
            static_assert((contains_type_v<std::remove_cvref_t<Ts>, components> && ...), "One or more view components are not part of this archetype");
            return archetype_view<const archetype_base, Ts...>(*this);
        }

    private:
        /**
         * @brief Pick the argument of type C from a parameter pack.
         *        Exactly one argument must have that type (enforced by static_assert above).
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
        std::tuple<vector<Components>...> m_storage;
    };

} // namespace tavros::core

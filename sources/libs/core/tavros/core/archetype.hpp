#pragma once

#include <tavros/core/containers/table.hpp>
#include <tavros/core/archetype_view.hpp>

namespace tavros::core
{

    /**
     * @brief Concept: type @p T is an archetype that contains all @p RequiredTs components.
     *
     * @tparam T           Archetype type to test.
     * @tparam RequiredTs  Component types that must be present in @p T.
     */
    template<class T, class... RequiredTs>
    concept archetype_with = requires {
        typename T::is_archetype;
    } && (are_unique_unqualified_v<type_list<RequiredTs...>>) && (is_subset_v<type_list<RequiredTs...>, class T::unqualified_types>);

    /**
     * @brief Structure-of-Arrays (SoA) storage for a fixed, unique set of component types.
     *
     * Inherits all row-management operations from @ref basic_table and adds
     * typed @ref view() factory methods that expose a subset of columns as an
     * @ref archetype_view.
     *
     * Compared to @ref basic_table, @c basic_archetype enforces that every
     * component type appears **exactly once** (via @c are_unique_unqualified_v),
     * making it suitable as the canonical per-archetype storage in an ECS.
     *
     * ### Example
     * @code
     *   basic_archetype<Position, Velocity, Health> arch;
     *   arch.typed_emplace_back(Position{0,0}, Velocity{1,0}, Health{100});
     *
     *   // Mutable view over two of the three columns
     *   auto v = arch.view<Position, Velocity>();
     *   for (auto [pos, vel] : v) {
     *       pos.x += vel.x;
     *   }
     *
     *   // Read-only view
     *   const auto& carch = arch;
     *   auto cv = carch.view<Health>();
     * @endcode
     *
     * @tparam Components Component types. Must all be distinct after cv-ref stripping.
     */
    template<class... Components>
        requires are_unique_unqualified_v<type_list<Components...>>
    class basic_archetype final : public basic_table<Components...>
    {
        using base = basic_table<Components...>;

    public:
        /// Marker type used by the @ref archetype_with concept to identify archetypes.
        using is_archetype = void;

        /// Inherit size and index type from base.
        using typename base::size_type;

        /** @brief Default-constructs an empty archetype with no rows. */
        basic_archetype() noexcept = default;

        /** @brief Default move-constructor. */
        basic_archetype(basic_archetype&& other) noexcept = default;

        /** @brief Default move-assignement operator. */
        basic_archetype& operator=(basic_archetype&& other) noexcept = default;

        /** @brief Default destructor. */
        ~basic_archetype() noexcept = default;

        /**
         * @brief Creates a mutable view over a subset of component columns.
         *
         * The returned @ref archetype_view holds a non-owning pointer to this
         * archetype; it is invalidated if the archetype is destroyed or if any
         * reallocation occurs (e.g. via @c emplace_back after capacity is exceeded).
         *
         * @tparam Ts  Components to expose. Each type must be present in
         *             @p Components and all must be distinct. May be
         *             cv-qualified to restrict mutability of individual columns
         *             (e.g. @c const Velocity).
         * @return     A lightweight @ref archetype_view<basic_archetype, Ts...>.
         *
         * @code
         *   auto v = arch.view<Position, Velocity>();
         *   v.each([](Position& p, Velocity& v) { p.x += v.x; });
         * @endcode
         */
        template<class... Ts>
        [[nodiscard]] auto view()
        {
            using unqualified_ts = type_transform_t<std::remove_cvref, type_list<Ts...>>;
            static_assert(is_subset_v<unqualified_ts, typename base::unqualified_types>, "One or more view components are not part of this archetype");
            return basic_archetype_view<basic_archetype, Ts...>(*this);
        }

        /**
         * @brief Creates an immutable view over a subset of component columns.
         *
         * All columns exposed by the view are implicitly const-qualified,
         * regardless of the cv-qualifiers on @p Ts.
         *
         * @tparam Ts  Components to expose. Same constraints as the mutable overload.
         * @return     A lightweight @ref archetype_view<const basic_archetype, const Ts...>.
         *
         * @code
         *   const auto& carch = arch;
         *   auto cv = carch.view<Position>();
         *   cv.each([](const Position& p) { /* read-only *\/ });
         * @endcode
         */
        template<class... Ts>
        [[nodiscard]] auto view() const
        {
            using unqualified_ts = type_transform_t<std::remove_cvref, type_list<Ts...>>;
            static_assert(is_subset_v<unqualified_ts, typename base::unqualified_types>, "One or more view components are not part of this archetype");
            return basic_archetype_view<basic_archetype, Ts...>(*this);
        }
    };

} // namespace tavros::core

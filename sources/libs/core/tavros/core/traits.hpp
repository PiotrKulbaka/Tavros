#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/fixed_string.hpp>
#include <type_traits>

namespace tavros::core
{

    /**
     * @brief Tag type representing a compile-time list of types.
     * @tparam Ts Parameter pack of types stored in the list.
     */
    template<class... Ts>
    struct type_list
    {
    };


    /**
     * @brief Retrieves the type at index @p I in a @ref type_list.
     * @tparam I Zero-based index.
     * @tparam List A @ref type_list specialization.
     * @note Ill-formed if @p I is out of bounds or @p List is not a @ref type_list.
     */
    template<size_t I, class List>
    struct type_at
    {
        static_assert(sizeof(List) == 0, "type_at: index out of bounds or second argument is not a type_list");
    };

    template<class T, class... Ts>
    struct type_at<0, type_list<T, Ts...>>
        : std::type_identity<T>
    {
    };

    template<size_t N, class T, class... Ts>
    struct type_at<N, type_list<T, Ts...>>
        : type_at<N - 1, Ts...>
    {
    };

    /** @brief Helper alias for @ref type_at. */
    template<size_t N, class List>
    using type_at_t = typename type_at<N, List>::type;


    /**
     * @brief Returns the number of types in a @ref type_list.
     * @tparam List A @ref type_list specialization.
     * @note Ill-formed if @p List is not a @ref type_list.
     */
    template<class List>
    struct list_size
    {
        static_assert(sizeof(List) == 0, "list_size: argument must be a type_list");
    };

    template<class... Ts>
    struct list_size<type_list<Ts...>>
        : std::integral_constant<size_t, sizeof...(Ts)>
    {
    };

    /** @brief Helper variable template for @ref list_size. */
    template<class List>
    inline constexpr size_t list_size_v = list_size<List>::value;


    /**
     * @brief Applies a unary metafunction @p Op to each type in a @ref type_list.
     * @tparam Op Unary template metafunction with a nested @c type alias (e.g. @c std::remove_cvref).
     * @tparam List A @ref type_list specialization.
     * @note Ill-formed if @p List is not a @ref type_list.
     */
    template<template<class> class Op, class List>
    struct type_transform
    {
        static_assert(sizeof(List) == 0, "type_transform: second argument must be a type_list");
    };

    template<template<class> class Op, class... Ts>
    struct type_transform<Op, type_list<Ts...>>
        : std::type_identity<type_list<typename Op<Ts>::type...>>
    {
    };

    /** @brief Helper alias for @ref type_transform. */
    template<template<class> class Op, class List>
    using type_transform_t = typename type_transform<Op, List>::type;


    /**
     * @brief Concatenates two @ref type_list specializations into one.
     * @tparam List1 First @ref type_list.
     * @tparam List2 Second @ref type_list.
     * @note Ill-formed if either argument is not a @ref type_list.
     */
    template<class List1, class List2>
    struct type_cat
    {
        static_assert(sizeof(List1) == 0, "type_cat: both arguments must be a type_list");
    };

    template<class... Ts, class... Us>
    struct type_cat<type_list<Ts...>, type_list<Us...>>
        : std::type_identity<type_list<Ts..., Us...>>
    {
    };

    /** @brief Helper alias for @ref type_cat. */
    template<class List1, class List2>
    using type_cat_t = typename type_cat<List1, List2>::type;


    /**
     * @brief Checks whether type @p T is present in a @ref type_list (exact match).
     * @tparam T Type to search for.
     * @tparam List A @ref type_list specialization.
     * @note Ill-formed if @p List is not a @ref type_list.
     */
    template<class T, class List>
    struct contains_type
    {
        static_assert(sizeof(T) == 0, "contains_type: second argument must be a type_list");
    };

    template<class T, class... Ts>
    struct contains_type<T, type_list<Ts...>>
        : std::bool_constant<(std::is_same_v<T, Ts> || ...)>
    {
    };

    /** @brief Helper variable template for @ref contains_type. */
    template<class T, class List>
    inline constexpr bool contains_type_v = contains_type<T, List>::value;


    /**
     * @brief Checks whether all types in @p List are contained in @p Other.
     * @tparam List Source @ref type_list - types to look up.
     * @tparam Other Target @ref type_list - types to search in.
     */
    template<class List, class Other>
    struct is_subset
    {
        static_assert(sizeof(List) == 0, "is_subset: both arguments must be a type_list");
    };

    template<class... Ts, class... Us>
    struct is_subset<type_list<Ts...>, type_list<Us...>>
        : std::conjunction<contains_type<Ts, type_list<Us...>>...>
    {
    };

    /** @brief Helper variable template for @ref is_subset. */
    template<class List, class Other>
    inline constexpr bool is_subset_v = is_subset<List, Other>::value;

    /**
     * @brief Checks whether all types in a @ref type_list are unique (exact match).
     * @tparam List A @ref type_list specialization.
     * @note Ill-formed if @p List is not a @ref type_list.
     */
    template<class List>
    struct are_unique
    {
        static_assert(sizeof(List) == 0, "are_unique: argument must be a type_list");
    };

    template<>
    struct are_unique<type_list<>> : std::true_type
    {
    };

    template<class T, class... Ts>
    struct are_unique<type_list<T, Ts...>>
        : std::bool_constant<!contains_type_v<T, type_list<Ts...>> && are_unique<type_list<Ts...>>::value>
    {
    };

    /** @brief Helper variable template for @ref are_unique. */
    template<class List>
    inline constexpr bool are_unique_v = are_unique<List>::value;


    /**
     * @brief Checks whether all types in a @ref type_list are unique after stripping cv-ref qualifiers.
     * @tparam List A @ref type_list specialization.
     * @note Equivalent to @ref are_unique after applying @c std::remove_cvref to each type.
     */
    template<class List>
    struct are_unique_unqualified
    {
        static_assert(sizeof(List) == 0, "are_unique_unqualified: argument must be a type_list");
    };

    template<class... Ts>
    struct are_unique_unqualified<type_list<Ts...>>
        : are_unique<type_transform_t<std::remove_cvref, type_list<Ts...>>>
    {
    };

    /** @brief Helper variable template for @ref are_unique_unqualified. */
    template<class List>
    inline constexpr bool are_unique_unqualified_v = are_unique_unqualified<List>::value;


    /**
     * @brief Checks whether all types in a @ref type_list are nothrow default-constructible.
     * @tparam List A @ref type_list specialization.
     * @note Ill-formed if @p List is not a @ref type_list.
     */
    template<class List>
    struct are_nothrow_default_constructible
    {
        static_assert(sizeof(List) == 0, "are_nothrow_default_constructible: argument must be a type_list");
    };

    template<class... Ts>
    struct are_nothrow_default_constructible<type_list<Ts...>>
        : std::bool_constant<(std::is_nothrow_default_constructible_v<Ts> && ...)>
    {
    };

    /** @brief Helper variable template for @ref are_nothrow_default_constructible. */
    template<class List>
    inline constexpr bool are_nothrow_default_constructible_v = are_nothrow_default_constructible<List>::value;


    /**
     * @brief Checks whether all types in a @ref type_list are nothrow-swappable.
     * @tparam List A @ref type_list specialization.
     * @note Ill-formed if @p List is not a @ref type_list.
     */
    template<class List>
    struct are_nothrow_swappable
    {
        static_assert(sizeof(List) == 0, "are_nothrow_swappable: argument must be a type_list");
    };

    template<class... Ts>
    struct are_nothrow_swappable<type_list<Ts...>>
        : std::bool_constant<(std::is_nothrow_swappable_v<Ts> && ...)>
    {
    };

    /** @brief Helper variable template for @ref are_nothrow_swappable. */
    template<class List>
    inline constexpr bool are_nothrow_swappable_v = are_nothrow_swappable<List>::value;


    /** @brief Checks whether a type is considered a string type. */
    template<typename T>
    struct is_string_type : std::false_type
    {
    };

    /** @brief Specialization for dynamic string type. */
    template<>
    struct is_string_type<string> : std::true_type
    {
    };

    /** @brief Specialization for fixed-capacity string type. */
    template<size_t N>
    struct is_string_type<core::fixed_string<N>> : std::true_type
    {
    };

    /** @brief Convenience variable template for @ref is_string_type. */
    template<typename T>
    inline constexpr bool is_string_type_v = is_string_type<T>::value;

} // namespace tavros::core

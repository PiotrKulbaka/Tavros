#pragma once

#include <concepts>
#include <fmt/fmt.hpp>
#include <tavros/core/ids/index_base.hpp>

namespace tavros::core
{

    using handle_type_t = uint16;
    using handle_gen_t = uint16;
    using handle_id_t = uint64;

    static_assert(
        sizeof(handle_type_t) + sizeof(handle_gen_t) + sizeof(index_t) == sizeof(handle_id_t),
        "handle_id_t must be exactly 64 bits: 16 type + 16 gen + 32 index"
    );


    /**
     * @brief Registration tag that associates a compile-time type ID with a handle type.
     *
     * Derive from this struct to register a new handle tag:
     * @code
     * struct texture_tag : handle_type_registration<1> {};
     * @endcode
     *
     * @tparam Id  Unique non-zero identifier for this handle type.
     *             Must be unique across all registered handle types in the application.
     */
    template<handle_type_t Id>
    struct handle_type_registration
    {
        static_assert(Id != 0, "Handle type ID 0 is reserved for invalid handles");

        using is_handle_type_registrator = void;

        /// @brief The compile-time type identifier value.
        static constexpr handle_type_t type_id_value = Id;
    };


    /**
     * @brief Concept that constrains a type to be a valid handle tag.
     *
     * A type satisfies this concept if it inherits from @ref handle_type_registration
     * and exposes a `type_id_value` constant of type @ref handle_type_t.
     *
     * @tparam T  Type to check.
     */
    template<class T>
    concept handle_tagged =
        requires {
            typename T::is_handle_type_registrator;
            requires std::same_as<decltype(T::type_id_value), const handle_type_t>;
        };

    /**
     * @brief Returns the compile-time type ID for a handle tag.
     * @tparam T  Type satisfying @ref handle_tagged.
     */
    template<handle_tagged T>
    constexpr handle_type_t type_id_v = T::type_id_value;

    /**
     * @brief Packs type ID, generation, and index into a single @ref handle_id_t.
     *
     * @param type_id     16-bit handle type identifier.
     * @param generation  16-bit generation counter.
     * @param index       32-bit slot index.
     * @return            Packed 64-bit handle ID.
     */
    constexpr handle_id_t make_handle_id(handle_type_t type_id, handle_gen_t generation, index_t index) noexcept
    {
        return (static_cast<handle_id_t>(type_id) << 48)
             | (static_cast<handle_id_t>(generation) << 32)
             | static_cast<handle_id_t>(index);
    }

    /**
     * @brief Extracts the type ID field from a packed handle ID.
     * @param id  Packed handle ID.
     * @return    16-bit type identifier.
     */
    constexpr handle_type_t handle_type_of(handle_id_t id) noexcept
    {
        return static_cast<handle_type_t>((id >> 48) & 0xffff);
    }

    /**
     * @brief Extracts the generation field from a packed handle ID.
     * @param id  Packed handle ID.
     * @return    16-bit generation counter.
     */
    constexpr handle_gen_t handle_gen_of(handle_id_t id) noexcept
    {
        return static_cast<handle_gen_t>((id >> 32) & 0xffff);
    }

    /**
     * @brief Extracts the index field from a packed handle ID.
     * @param id  Packed handle ID.
     * @return    32-bit slot index.
     */
    constexpr index_t handle_index_of(handle_id_t id) noexcept
    {
        return static_cast<index_t>(id & 0xffffffff);
    }


    /**
     * @brief Strongly-typed handle with embedded type ID, generation, and index.
     *
     * Encodes all three fields into a single 64-bit value for cache-friendly storage.
     * The @p Tag parameter prevents accidental mixing of handles from different systems
     * (e.g. passing a `texture_handle` where a `buffer_handle` is expected).
     *
     * @par Validity
     * A handle is considered valid if its raw ID is non-zero (@ref valid, @ref operator bool).
     * Validity does not imply liveness - the slot may have been deallocated and reallocated
     * with a higher generation. Use @ref handle_allocator::contains for liveness checks.
     *
     * @par Equality
     * Two handles are equal only if both are valid and their IDs match exactly (type,
     * generation, and index all equal). Two invalid handles are @b not considered equal.
     * @see operator==
     *
     * @tparam Tag  Tag type satisfying @ref handle_tagged. Uniquely identifies the handle family.
     */
    template<handle_tagged Tag>
    struct handle_base
    {
        static constexpr handle_type_t k_type_id = type_id_v<Tag>;

        /// Packed 64-bit handle ID. Zero means invalid.
        handle_id_t id = 0;

        /// Constructs an invalid (null) handle.
        handle_base() noexcept = default;

        /**
         * @brief Constructs a handle from a raw packed ID.
         * @param raw_id  Pre-packed 64-bit handle ID.
         */
        explicit handle_base(handle_id_t raw_id)
            : id(raw_id)
        {
        }

        /**
         * @brief Constructs a handle by packing generation and index with the tag's type ID.
         * @param gen    Generation counter for this slot.
         * @param index  Slot index within the allocator.
         */
        explicit handle_base(handle_gen_t gen, index_t index)
            : id(make_handle_id(k_type_id, gen, index))
        {
        }

        /**
         * @brief Returns the type ID embedded in this handle.
         */
        constexpr handle_type_t type() const noexcept
        {
            return handle_type_of(id);
        }

        /**
         * @brief Returns the generation counter embedded in this handle.
         */
        constexpr handle_gen_t generation() const noexcept
        {
            return handle_gen_of(id);
        }

        /**
         * @brief Returns the slot index embedded in this handle.
         */
        constexpr index_t index() const noexcept
        {
            return handle_index_of(id);
        }

        /**
         * @brief Returns @c true if the handle is non-null.
         *
         * A non-null handle was issued by an allocator and has not been explicitly
         * zeroed. It may still be stale if the underlying slot was deallocated.
         */
        constexpr bool valid() const noexcept
        {
            return id != 0;
        }

        /**
         * @brief Allows checking validity in boolean context.
         * @example if (handle) { ... }
         */
        explicit operator bool() const noexcept
        {
            return valid();
        }
    };

    /**
     * @brief Equality comparison for handles of the same tag type.
     *
     * @return @c true if both handles are valid and their packed IDs are identical.
     */
    template<handle_tagged Tag>
    constexpr bool operator==(handle_base<Tag> lhs, handle_base<Tag> rhs) noexcept
    {
        return lhs.id == rhs.id;
    }

    /**
     * @brief Inequality comparison for handles of the same tag type.
     * @see operator==
     */
    template<handle_tagged Tag>
    constexpr bool operator!=(handle_base<Tag> lhs, handle_base<Tag> rhs) noexcept
    {
        return !(lhs == rhs);
    }

    template<typename T>
    concept is_handle =
        std::default_initializable<T> && requires(T a, T b) {
            { static_cast<bool>(a) } -> std::same_as<bool>;
            { a == b } -> std::same_as<bool>;
            { a != b } -> std::same_as<bool>;
            { a.generation() } -> std::convertible_to<std::size_t>;
            { a.index() } -> std::convertible_to<std::size_t>;
        };

} // namespace tavros::core

/**
 * @brief fmt formatter for @ref tavros::core::handle_base.
 *
 * Formats a handle as `TTTT:GGGG:IIIIIIII` where:
 * - `TTTT`     - 4 hex digits of type ID
 * - `GGGG`     - 4 hex digits of generation
 * - `IIIIIIII` - 8 hex digits of index
 *
 * Invalid handles are formatted with error styling applied to all fields.
 */
template<class Tag>
struct fmt::formatter<tavros::core::handle_base<Tag>>
{
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const tavros::core::handle_base<Tag>& h, FormatContext& ctx) const
    {
        char ty[5] = {};
        char gen[5] = {};
        char idx[9] = {};
        to_hex(h.type(), 4, ty);
        to_hex(h.generation(), 4, gen);
        to_hex(h.index(), 8, idx);

        if (!h) {
            return fmt::format_to(ctx.out(), "(invalid) {}:{}:{}", fmt::styled_error(ty), fmt::styled_error(gen), fmt::styled_error(idx));
        }

        return fmt::format_to(ctx.out(), "{}:{}:{}", fmt::styled_important(ty), fmt::styled_name(gen), fmt::styled_important(idx));
    }

private:
    static constexpr void to_hex(uint64 num, size_t count, char* dst) noexcept
    {
        static constexpr char k_alpha[] = "0123456789abcdef";
        for (size_t i = 0; i < count; ++i) {
            dst[count - i - 1] = k_alpha[num & 0xf];
            num >>= 4;
        }
        dst[count] = '\0';
    }
};

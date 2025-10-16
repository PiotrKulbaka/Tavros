#pragma once

#include <tavros/core/types.hpp>
#include <fmt/fmt.hpp>

namespace tavros::core
{

    /**
     * @brief Lightweight handle for a objects in a object_pool.
     *
     * Encapsulates an ID that contains index and generation to safely refer
     * to objects in a object_pool.
     */
    template<class ObjectTag>
    struct object_handle
    {
        using id_t = uint64;

        id_t id = 0xffffffffffffffffui64;

        object_handle() noexcept = default;

        explicit object_handle(id_t handle_id)
            : id(handle_id)
        {
        }

        bool constexpr valid() const noexcept
        {
            return id != 0xffffffffffffffffui64;
        }

        /**
         * @brief Allows checking validity in boolean context.
         * @example if (handle) { ... }
         */
        explicit operator bool() const noexcept
        {
            return valid();
        }

        constexpr bool operator==(object_handle other) const noexcept
        {
            return id == other.id;
        }

        constexpr bool operator!=(object_handle other) const noexcept
        {
            return id != other.id;
        }
    };

} // namespace tavros::core

template<typename ObjectTag>
struct fmt::formatter<tavros::core::object_handle<ObjectTag>>
{
    fmt::formatter<uint64_t> m_base;

    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return m_base.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const tavros::core::object_handle<ObjectTag>& h, FormatContext& ctx) const
    {
        if (!h) {
            return fmt::format_to(ctx.out(), "{} {}", fmt::styled_error("(invalid)"), fmt::styled_important(tavros::core::uint64_to_base64(h.id)));
        }
        return fmt::format_to(ctx.out(), "{}", fmt::styled_important(tavros::core::uint64_to_base64(h.id)));
    }
};

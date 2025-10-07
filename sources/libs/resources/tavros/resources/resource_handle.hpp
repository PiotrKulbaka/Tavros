#pragma once

#include <tavros/core/types.hpp>
#include <fmt/fmt.hpp>

namespace tavros::resources
{

    /**
     * @brief Lightweight handle for a resource in a resource_pool.
     *
     * Encapsulates an ID that contains index and generation to safely refer
     * to objects in a resource_pool.
     */
    template<class T>
    struct resource_handle
    {
        uint32 id = 0xffffffffui32;

        /**
         * @brief Returns an invalid handle.
         */
        static constexpr resource_handle invalid() noexcept
        {
            return {0xffffffffui32};
        }

        constexpr bool operator==(resource_handle other) const noexcept
        {
            return id == other.id;
        }

        constexpr bool operator!=(resource_handle other) const noexcept
        {
            return id != other.id;
        }
    };

} // namespace tavros::resources

template<typename ResourceTag>
struct fmt::formatter<tavros::resources::resource_handle<ResourceTag>>
{
    fmt::formatter<uint32_t> m_base;

    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return m_base.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const tavros::resources::resource_handle<ResourceTag>& h, FormatContext& ctx) const
    {
        if (h == tavros::resources::resource_handle<ResourceTag>::invalid()) {
            return fmt::format_to(ctx.out(), "{}", fmt::styled_error("(invalid)"));
        }
        return fmt::format_to(ctx.out(), "{}", fmt::styled_important(tavros::core::uint32_to_base64(h.id)));
    }
};

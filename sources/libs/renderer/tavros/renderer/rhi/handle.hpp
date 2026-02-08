#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/utils/to_string.hpp>
#include <fmt/fmt.hpp>

namespace tavros::renderer::rhi
{

    template<typename ObjectTag>
    struct handle_base
    {
        static constexpr uint64 invalid_value_id = 0xffffffffffffffffui64;

        uint64 id = invalid_value_id;

        handle_base() noexcept = default;

        explicit handle_base(uint64 handle_id)
            : id(handle_id)
        {
        }

        ~handle_base() noexcept
        {
            id = invalid_value_id;
        }

        handle_base(const handle_base&) noexcept = default;

        handle_base& operator=(const handle_base&) noexcept = default;

        /**
         * @brief Move constructor.
         *
         * Takes ownership of the handle from another object.
         * The moved-from handle is left in an invalid state.
         */
        handle_base(handle_base&& other) noexcept
            : id(other.id)
        {
            // Explicitly release ownership from the source
            other.id = invalid_value_id;
        }

        /**
         * @brief Move assignment operator.
         *
         * Transfers ownership of the handle.
         * The moved-from handle is left in an invalid state.
         */
        handle_base& operator=(handle_base&& other) noexcept
        {
            if (this != &other) {
                id = other.id;

                // Explicitly release ownership from the source
                other.id = invalid_value_id;
            }

            return *this;
        }

        bool constexpr valid() const noexcept
        {
            return id != invalid_value_id;
        }

        /**
         * @brief Allows checking validity in boolean context.
         * @example if (handle) { ... }
         */
        explicit operator bool() const noexcept
        {
            return valid();
        }

        constexpr bool operator==(handle_base other) const noexcept
        {
            return id == other.id;
        }

        constexpr bool operator!=(handle_base other) const noexcept
        {
            return id != other.id;
        }
    };

    struct frame_composer_tag
    {
    };
    struct sampler_tag
    {
    };
    struct texture_tag
    {
    };
    struct pipeline_tag
    {
    };
    struct framebuffer_tag
    {
    };
    struct buffer_tag
    {
    };
    struct render_pass_tag
    {
    };
    struct shader_binding_tag
    {
    };
    struct shader_tag
    {
    };
    struct fence_tag
    {
    };

    using frame_composer_handle = handle_base<frame_composer_tag>;
    using sampler_handle = handle_base<sampler_tag>;
    using texture_handle = handle_base<texture_tag>;
    using pipeline_handle = handle_base<pipeline_tag>;
    using framebuffer_handle = handle_base<framebuffer_tag>;
    using buffer_handle = handle_base<buffer_tag>;
    using render_pass_handle = handle_base<render_pass_tag>;
    using shader_binding_handle = handle_base<shader_binding_tag>;
    using shader_handle = handle_base<shader_tag>;
    using fence_handle = handle_base<fence_tag>;

} // namespace tavros::renderer::rhi

template<typename ObjectTag>
struct fmt::formatter<tavros::renderer::rhi::handle_base<ObjectTag>>
{
    fmt::formatter<uint64_t> m_base;

    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return m_base.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const tavros::renderer::rhi::handle_base<ObjectTag>& h, FormatContext& ctx) const
    {
        if (!h) {
            return fmt::format_to(ctx.out(), "{} {}", fmt::styled_error("(invalid)"), fmt::styled_important(tavros::core::uint64_to_base64(h.id)));
        }
        return fmt::format_to(ctx.out(), "{}", fmt::styled_important(tavros::core::uint64_to_base64(h.id)));
    }
};

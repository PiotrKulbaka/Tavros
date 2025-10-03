#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/utils/to_string.hpp>
#include <fmt/fmt.hpp>

namespace tavros::renderer::rhi
{

    template<typename ResourceTag>
    struct handle_base
    {
        uint32 id = 0xffffffffui32;

        static constexpr handle_base invalid() noexcept
        {
            return {0xffffffffui32};
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
    struct geometry_tag
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

    using frame_composer_handle = handle_base<frame_composer_tag>;
    using sampler_handle = handle_base<sampler_tag>;
    using texture_handle = handle_base<texture_tag>;
    using pipeline_handle = handle_base<pipeline_tag>;
    using framebuffer_handle = handle_base<framebuffer_tag>;
    using buffer_handle = handle_base<buffer_tag>;
    using geometry_handle = handle_base<geometry_tag>;
    using render_pass_handle = handle_base<render_pass_tag>;
    using shader_binding_handle = handle_base<shader_binding_tag>;
    using shader_handle = handle_base<shader_tag>;

} // namespace tavros::renderer::rhi

template<typename ResourceTag>
struct fmt::formatter<tavros::renderer::rhi::handle_base<ResourceTag>>
{
    fmt::formatter<uint32_t> m_base;

    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return m_base.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const tavros::renderer::rhi::handle_base<ResourceTag>& h, FormatContext& ctx) const
    {
        if (h == tavros::renderer::rhi::handle_base<ResourceTag>::invalid()) {
            return fmt::format_to(ctx.out(), "{}", fmt::styled_error("(invalid)"));
        }
        return fmt::format_to(ctx.out(), "{}", fmt::styled_important(tavros::core::uint32_to_base64(h.id)));
    }
};

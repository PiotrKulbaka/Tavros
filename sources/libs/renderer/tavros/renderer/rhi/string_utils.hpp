#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/renderer/rhi/enums.hpp>
#include <fmt/fmt.hpp>

namespace tavros::renderer::rhi
{

    core::string_view to_string(buffer_usage val) noexcept;
    core::string_view to_string(buffer_access val) noexcept;
    core::string_view to_string(index_buffer_format val) noexcept;
    core::string_view to_string(scalar_type val) noexcept;
    core::string_view to_string(composite_format val) noexcept;
    core::string_view to_string(primitive_topology val) noexcept;
    core::string_view to_string(compare_op val) noexcept;
    core::string_view to_string(stencil_op val) noexcept;
    core::string_view to_string(blend_factor val) noexcept;
    core::string_view to_string(blend_op val) noexcept;
    core::string_view to_string(color_mask val) noexcept;
    core::string_view to_string(cull_face val) noexcept;
    core::string_view to_string(front_face val) noexcept;
    core::string_view to_string(polygon_mode val) noexcept;
    core::string_view to_string(pixel_format val) noexcept;
    core::string_view to_string(texture_type val) noexcept;
    core::string_view to_string(texture_usage val) noexcept;
    core::string_view to_string(filter_mode val) noexcept;
    core::string_view to_string(mipmap_filter_mode val) noexcept;
    core::string_view to_string(wrap_mode val) noexcept;
    core::string_view to_string(render_backend_type val) noexcept;
    core::string_view to_string(load_op val) noexcept;
    core::string_view to_string(store_op val) noexcept;
    core::string_view to_string(shader_resource_type val) noexcept;

    pixel_format combine_depth_stencil_formats(pixel_format df, pixel_format sf) noexcept;

} // namespace tavros::renderer::rhi

template<typename EnumType>
struct fmt::formatter<EnumType, char, std::enable_if_t<std::is_enum_v<EnumType>>>
{
    fmt::formatter<tavros::core::string_view> m_base;

    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return m_base.parse(ctx);
    }

    template<typename FormatContext>
    auto format(EnumType e, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", fmt::styled_name(tavros::renderer::rhi::to_string(e)));
    }
};


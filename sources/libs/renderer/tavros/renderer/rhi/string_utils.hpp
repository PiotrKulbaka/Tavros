#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>
#include <fmt/fmt.hpp>

namespace tavros::renderer::rhi
{

    enum class buffer_usage : uint8;
    enum class buffer_access : uint8;
    enum class index_buffer_format : uint8;
    enum class attribute_format : uint8;
    enum class attribute_type : uint8;
    enum class primitive_topology : uint8;
    enum class compare_op : uint8;
    enum class stencil_op : uint8;
    enum class blend_factor : uint8;
    enum class blend_op : uint8;
    enum class color_mask : uint8;
    enum class cull_face : uint8;
    enum class front_face : uint8;
    enum class polygon_mode : uint8;
    enum class pixel_format : uint8;
    enum class texture_type : uint8;
    enum class texture_usage : uint8;
    enum class filter_mode : uint8;
    enum class mipmap_filter_mode : uint8;
    enum class wrap_mode : uint8;
    enum class shader_stage : uint8;
    enum class render_backend_type : uint8;
    enum class load_op : uint8;
    enum class store_op : uint8;

    core::string_view to_string(buffer_usage val) noexcept;
    core::string_view to_string(buffer_access val) noexcept;
    core::string_view to_string(index_buffer_format val) noexcept;
    core::string_view to_string(attribute_format val) noexcept;
    core::string_view to_string(attribute_type val) noexcept;
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
    core::string_view to_string(shader_stage val) noexcept;
    core::string_view to_string(render_backend_type val) noexcept;
    core::string_view to_string(load_op val) noexcept;
    core::string_view to_string(store_op val) noexcept;

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


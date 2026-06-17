#pragma once

#include <tavros/core/fixed_string.hpp>
#include <tavros/tef/workspace.hpp>
#include <tavros/tef/schema.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/resources/resource_desc.hpp>

namespace tavros::renderer
{

    class render_target_desc : public resource_desc_base<render_target_desc>
    {
    public:
        struct color_attachment_config
        {
            core::fixed_string<63> name = "";
            rhi::pixel_format      format = rhi::pixel_format::none;
            rhi::load_op           load = rhi::load_op::dont_care;
            rhi::store_op          store = rhi::store_op::dont_care;
            float                  clear_value[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        };

        using color_attachments_config = core::fixed_vector<color_attachment_config, rhi::k_max_color_attachments>;

        struct depth_attachment_config
        {
            rhi::pixel_format format = rhi::pixel_format::none;
            rhi::load_op      load = rhi::load_op::dont_care;
            rhi::store_op     store = rhi::store_op::dont_care;
            float             clear_value = 1.0f;
        };

        struct stencil_attachment_config
        {
            rhi::pixel_format format = rhi::pixel_format::none;
            rhi::load_op      load = rhi::load_op::dont_care;
            rhi::store_op     store = rhi::store_op::dont_care;
            uint32            clear_value = 0;
        };

    public:
        render_target_desc() noexcept
            : resource_desc_base("")
            , m_hash_cache(0)
        {
        }

        render_target_desc(
            core::string_view                rt_name,
            const color_attachments_config&  ca,
            const depth_attachment_config&   da,
            const stencil_attachment_config& sa
        ) noexcept
            : resource_desc_base(rt_name)
            , m_color_configs(ca)
            , m_depth_config(da)
            , m_stencil_config(sa)
        {
            m_hash_cache = std::hash<core::string_view>{}(name());
        }

        ~render_target_desc() noexcept = default;

        const color_attachments_config& color_attachments() const noexcept
        {
            return m_color_configs;
        }

        const depth_attachment_config& depth_attachment() const noexcept
        {
            return m_depth_config;
        }

        const stencil_attachment_config& stencil_attachment() const noexcept
        {
            return m_stencil_config;
        }

    public:
        bool is_valid_impl() const noexcept
        {
            if (name().empty()) {
                return false;
            }

            const bool has_color = !m_color_configs.empty();
            const bool has_depth = m_depth_config.format != rhi::pixel_format::none;
            const bool has_stencil = m_stencil_config.format != rhi::pixel_format::none;
            if (!has_color && !has_depth && !has_stencil) {
                return false;
            }

            for (const auto& ca : m_color_configs) {
                if (ca.name.empty() || ca.format == rhi::pixel_format::none) {
                    return false;
                }
            }

            return true;
        }

        size_t hash_impl() const noexcept
        {
            return m_hash_cache;
        }

        bool equals_impl(const render_target_desc& other) const noexcept
        {
            return name() == other.name();
        }

    private:
        color_attachments_config  m_color_configs;
        depth_attachment_config   m_depth_config;
        stencil_attachment_config m_stencil_config;
        size_t                    m_hash_cache = 0;
    };

} // namespace tavros::renderer

namespace tavros::tef
{
    template<>
    struct schema<tavros::renderer::render_target_desc>
    {
        static void serialize(node* n, const tavros::renderer::render_target_desc& in, core::diagnostics& ds) noexcept;
        static void deserialize(const node* n, tavros::renderer::render_target_desc& out, core::diagnostics& ds) noexcept;
    };
} // namespace tavros::tef

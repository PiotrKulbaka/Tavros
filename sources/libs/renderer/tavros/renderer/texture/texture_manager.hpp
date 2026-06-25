#pragma once

#include <tavros/core/resource/resource_manager.hpp>
#include <tavros/assets/image/image_view.hpp>
#include <tavros/assets/asset_manager.hpp>
#include <tavros/renderer/texture/texture.hpp>
#include <tavros/renderer/texture/texture_desc.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

#include <tavros/tef/workspace.hpp>

#include <tavros/renderer/upload_context.hpp>

namespace tavros::renderer
{

    class texture_manager : public core::basic_resource_manager<texture>
    {
    public:
        texture_manager(rhi::graphics_device* gdevice, assets::asset_manager* am) noexcept;

        ~texture_manager() noexcept;

        resource_ref_type load(upload_context& upctx, core::string_view name, tef::workspace& ws);

        resource_ref_type load(upload_context& upctx, const texture_desc& desc);

        resource_ref_type load(upload_context& upctx, assets::image_view im, const texture_desc& desc);

    private:
        rhi::graphics_device*  m_gdevice = nullptr;
        assets::asset_manager* m_am = nullptr;
    };

} // namespace tavros::renderer

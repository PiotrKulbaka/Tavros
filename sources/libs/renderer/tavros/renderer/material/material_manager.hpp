#pragma once

#include <tavros/core/resource/resource_manager.hpp>
#include <tavros/assets/asset_manager.hpp>
#include <tavros/renderer/material/material.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/tef/workspace.hpp>

namespace tavros::renderer
{

    class material_manager : public core::basic_resource_manager<material>
    {
    public:
        material_manager(rhi::graphics_device* gdevice, assets::asset_manager* am);

        ~material_manager() noexcept;

        resource_ref_type load(core::string_view name, tef::workspace& ws, uint32 msaa, rhi::pixel_format ds_format);

        resource_ref_type create(const material_desc& desc, uint32 msaa, rhi::pixel_format ds_format);

    private:
        rhi::graphics_device*  m_gdevice = nullptr;
        assets::asset_manager* m_am = nullptr;
        shader_loader          m_sl;
    };

} // namespace tavros::renderer

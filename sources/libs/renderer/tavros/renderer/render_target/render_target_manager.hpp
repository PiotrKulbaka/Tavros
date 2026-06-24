#pragma once

#include <tavros/core/resource/resource_manager.hpp>
#include <tavros/renderer/render_target/render_target.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/tef/workspace.hpp>

namespace tavros::renderer
{

    class render_target_manager : public core::basic_resource_manager<render_target>
    {
    public:
        render_target_manager(rhi::graphics_device* gdevice) noexcept;

        ~render_target_manager() noexcept override;

        void release_resource(render_target& res) noexcept override;

        resource_ref_type load(core::string_view name, tef::workspace& ws);

        resource_ref_type create(const render_target_desc& desc);

    private:
        rhi::graphics_device* m_gdevice = nullptr;
    };

} // namespace tavros::renderer

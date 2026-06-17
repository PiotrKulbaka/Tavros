#pragma once

#include <tavros/renderer/tags.hpp>
#include <tavros/core/resource/resource_registry.hpp>
#include <tavros/renderer/material/material.hpp>
#include <tavros/renderer/render_target/render_target_registry.hpp>
#include <tavros/assets/asset_manager.hpp>

namespace tavros::renderer
{

    class resource_manager;

    /// @brief CPU-side metadata for a GPU texture resource.
    struct gpu_material_view
    {
        rhi::pipeline_handle gpu_handle;
    };

    class material_registry : public core::resource_registry<gpu_material_view, material_tag, material_registry>
    {
    public:
        /** @brief Constructs the material manager. */
        explicit material_registry(resource_manager* rm) noexcept;

        /** @brief Default destructor. */
        ~material_registry() noexcept = default;

        handle_type create(const tef::workspace& ws, render_target_handle rt_handle, core::string_view mt_path);

        handle_type create(core::string_view mt_name, render_target_handle rt_handle, const material_desc& desc);

    public:
        void release_resource(gpu_material_view& mt) noexcept;

    private:
        /// Non-owning pointer to the parent resource manager
        resource_manager* m_rm = nullptr;
    };

} // namespace tavros::renderer

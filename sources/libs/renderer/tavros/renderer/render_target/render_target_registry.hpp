#pragma once

#include <tavros/core/resource/resource_registry.hpp>
#include <tavros/core/math.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/gpu_stage_buffer.hpp>
#include <tavros/renderer/tags.hpp>
#include <tavros/renderer/render_target/render_target.hpp>
#include <tavros/assets/asset_manager.hpp>

namespace tavros::renderer
{

    class resource_manager;

    using rt_resource = std::pair<render_target_desc, render_target>;

    /**
     */
    class render_target_registry : public core::resource_registry<rt_resource, render_target_tag, render_target_registry>
    {
    public:
        /**
         * @brief Constructs the texture manager.
         * @param am Asset manager used to read image files from disk. Must not be null.
         */
        explicit render_target_registry(resource_manager* rrm) noexcept;

        /** @brief Default destructor. */
        ~render_target_registry() noexcept = default;

        handle_type create(const tef::workspace& ws, core::string_view rt_path);

        handle_type create(core::string_view rt_name, const render_target_desc& desc);

    public:
        void release_resource(rt_resource& tex_view) noexcept
        {
        }

    private:
        /// Non-owning pointer to the parent resource manager
        resource_manager* m_rrm = nullptr;
    };

    /// @brief Opaque handle to a texture managed by texture_manager.
    using render_target_handle = render_target_registry::handle_type;

} // namespace tavros::renderer

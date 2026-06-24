#include <tavros/renderer/render_target/render_target_manager.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("render_target_manager");
}

namespace tavros::renderer
{

    render_target_manager::render_target_manager(rhi::graphics_device* gdevice) noexcept
        : m_gdevice(gdevice)
    {
    }

    render_target_manager::~render_target_manager() noexcept
    {
        clear();
    }


    void render_target_manager::release_resource(render_target& res) noexcept
    {
    }

    render_target_manager::resource_ref_type render_target_manager::load(core::string_view name, tef::workspace& ws)
    {
        render_target_desc desc;
        core::diagnostics  ds;

        tavros::tef::schema<render_target_desc>::deserialize(ws.resolve_path(name), desc, ds);
        if (ds.error_count() > 0 || ds.fatal_count() > 0) {
            logger.error("Failed to load named render_target '{}'", name);
            logger.flush(ds);
            return {};
        }

        if (ds.total_count() > 0) {
            logger.flush(ds);
        }

        return create(desc);
    }

    render_target_manager::resource_ref_type render_target_manager::create(const render_target_desc& desc)
    {
        auto ref = insert(render_target(m_gdevice, desc));
        if (!ref) {
            logger.error("Failed to create render target '{}'", desc.name());
            return {};
        }

        logger.debug("Render taget {}: created.", desc.name());
        return ref;
    }

} // namespace tavros::renderer

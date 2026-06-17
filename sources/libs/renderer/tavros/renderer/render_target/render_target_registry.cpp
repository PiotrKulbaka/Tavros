#include <tavros/renderer/render_target/render_target_registry.hpp>

#include <tavros/renderer/render_target/render_target.hpp>
#include <tavros/renderer/resource_manager.hpp>
#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("render_target_registry");
}

namespace tavros::renderer
{

    render_target_registry::render_target_registry(resource_manager* rrm) noexcept
        : m_rrm(rrm)
    {
        TAV_ASSERT(rrm);
    }

    render_target_handle render_target_registry::create(const tef::workspace& ws, core::string_view rt_path)
    {
        if (auto h = find_handle(rt_path); h.valid()) {
            // Already loaded
            acquire(h);
            return h;
        }

        core::diagnostics ds;
        auto*             n = ws.resolve_path(rt_path);
        if (!n) {
            ds.error("Render target configuration not found at '{}'.", rt_path);
            logger.error("Failed to create render target '{}': {}", rt_path, ds.text());
            return {};
        }

        render_target_desc desc;
        tef::schema<render_target_desc>::deserialize(n, desc, ds);
        if (ds.error_count() || ds.fatal_count()) {
            logger.error("Failed to create render target '{}': {}", rt_path, ds.text());
            return {};
        }

        logger.flush(ds);

        auto handle = insert(rt_path, std::make_pair(desc, render_target(m_rrm->graphics_device(), desc)));
        if (!handle.valid()) {
            logger.error("Failed to insert render target '{}'", rt_path);
            return {};
        }

        return handle;
    }

    render_target_handle render_target_registry::create(core::string_view rt_name, const render_target_desc& desc)
    {
        if (auto h = find_handle(rt_name); h.valid()) {
            // Already loaded
            acquire(h);
            return h;
        }

        auto handle = insert(rt_name, std::make_pair(desc, render_target(m_rrm->graphics_device(), desc)));
        if (!handle.valid()) {
            logger.error("Failed to insert render target '{}'", rt_name);
            return {};
        }

        return handle;
    }

} // namespace tavros::renderer

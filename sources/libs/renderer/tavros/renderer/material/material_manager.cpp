#include <tavros/renderer/material/material_manager.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("material_manager");

    class shader_provider : public tavros::renderer::shader_source_provider
    {
    public:
        explicit shader_provider(tavros::assets::asset_manager* am)
            : m_am(am)
        {
        }

        tavros::core::string load(tavros::core::string_view path) override
        {
            return m_am->read_text(path);
        }

    private:
        tavros::assets::asset_manager* m_am;
    };
} // namespace

namespace tavros::renderer
{

    material_manager::material_manager(rhi::graphics_device* gdevice, assets::asset_manager* am)
        : m_gdevice(gdevice)
        , m_am(am)
        , m_sl(std::move(core::make_unique<shader_provider>(am)))
    {
    }

    material_manager::~material_manager() noexcept
    {
        clear();
    }

    material_manager::resource_ref_type material_manager::load(core::string_view name, tef::workspace& ws, uint32 msaa, rhi::pixel_format ds_format)
    {
        if (auto res = find(name); res) {
            // Already loaded
            acquire(res);
            return res;
        }

        material_desc     desc;
        core::diagnostics ds;

        tavros::tef::schema<material_desc>::deserialize(ws.resolve_path(name), desc, ds);
        if (ds.error_count() > 0 || ds.fatal_count() > 0) {
            logger.error("Failed to load material '{}'", name);
            logger.flush(ds);
            return {};
        }

        if (ds.total_count() > 0) {
            logger.flush(ds);
        }

        auto mt = material(m_gdevice, desc, m_sl, msaa, ds_format);
        if (mt.is_valid()) {
            auto ref = insert(std::move(mt));
            if (!ref) {
                logger.error("Failed to insert material '{}'", name);
            }
            return ref;
        }

        logger.error("Failed to create material '{}'", name);
        return {};
    }

    material_manager::resource_ref_type material_manager::create(const material_desc& desc, uint32 msaa, rhi::pixel_format ds_format)
    {
        auto name = desc.name();
        if (auto res = find(name); res) {
            // Already loaded
            acquire(res);
            return res;
        }

        auto mt = material(m_gdevice, desc, m_sl, msaa, ds_format);
        if (mt.is_valid()) {
            auto ref = insert(std::move(mt));
            if (!ref) {
                logger.error("Failed to insert material '{}'", name);
            }
            return ref;
        }

        logger.error("Failed to create material '{}'", name);
        return {};
    }

} // namespace tavros::renderer

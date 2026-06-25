#include <tavros/renderer/resource_manager.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("resource_manager");

    class fs_shader_provider : public tavros::renderer::shader_source_provider
    {
    public:
        explicit fs_shader_provider(tavros::core::shared_ptr<tavros::assets::asset_manager> am)
            : m_am(std::move(am))
        {
        }

        tavros::core::string load(tavros::core::string_view path) override
        {
            return m_am->read_text(path);
        }

    private:
        tavros::core::shared_ptr<tavros::assets::asset_manager> m_am;
    };

} // namespace

namespace tavros::renderer
{

    resource_manager::resource_manager(rhi::graphics_device* gdevice, core::shared_ptr<assets::asset_manager> am, core::shared_ptr<tef::workspace> ws) noexcept
        : m_gdevice(gdevice)
        , m_am(am)
        , m_ws(ws)
        , m_tex_mgr(gdevice, am.get())
        , m_rt_mgr(gdevice)
        , m_mt_mgr(gdevice, am.get())
        , m_upctx(nullptr)
    {
    }

    resource_manager::~resource_manager() noexcept
    {
    }

    void resource_manager::init(upload_context* upctx) noexcept
    {
        if (m_upctx) {
            logger.error("Resource manager already initialzied.");
            return;
        }
        m_upctx = upctx;
    }

    void resource_manager::shutdown() noexcept
    {
        if (!m_upctx) {
            logger.error("Resource manager is not initialzied.");
            return;
        }
        m_upctx = nullptr;
        m_tex_mgr.clear();
    }

    void resource_manager::begin_frame() noexcept
    {
    }

    void resource_manager::end_frame() noexcept
    {
    }
} // namespace tavros::renderer

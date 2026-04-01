#include <tavros/renderer/mesh/mesh_manager.hpp>

namespace tavros::renderer
{

    mesh_manager::mesh_manager(core::shared_ptr<assets::asset_manager> am) noexcept
        : m_am(std::move(am))
    {
        TAV_ASSERT(m_am);
    }

    mesh_manager::~mesh_manager() noexcept
    {
    }

    void mesh_manager::init(rhi::graphics_device* device) noexcept
    {
        TAV_ASSERT(device);
        TAV_ASSERT(!m_gdevice);
        m_gdevice = device;
    }

    void mesh_manager::shutdown() noexcept
    {
        TAV_ASSERT(m_gdevice);
        clear();
        m_gdevice = nullptr;
    }

} // namespace tavros::renderer
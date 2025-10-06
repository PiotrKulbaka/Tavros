#include "resource_locator.hpp"

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger{"resource_locator"};
}

namespace app
{

    resource_locator::resource_locator(tavros::core::string_view assets_root)
        : m_assets_root(assets_root)
    {
        if (!std::filesystem::exists(m_assets_root)) {
            ::logger.error("Assets root does not exist: {}", assets_root);
        }
    }

    std::filesystem::path resource_locator::resolve_texture(tavros::core::string_view relative_path) const
    {
        std::filesystem::path rel(relative_path.data());
        std::filesystem::path full = m_assets_root / "textures" / rel;

        full = std::filesystem::weakly_canonical(full);

        if (!std::filesystem::exists(full)) {
            ::logger.warning("Resource does not exist: {}", full.string());
        }

        return full;
    }

} // namespace app

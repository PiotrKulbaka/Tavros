#include <tavros/resources/resource_manager.hpp>

#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::core::logger logger("resource_manager");
}

namespace tavros::resources
{

    resource_manager::resource_manager() = default;

    resource_manager::~resource_manager() = default;

    bool resource_manager::exists(core::string_view path) const
    {
        for (auto& p : m_providers) {
            if (p->exists(path)) {
                return true;
            }
        }
        return false;
    }

    core::shared_ptr<resource> resource_manager::open(core::string_view path)
    {
        for (auto& p : m_providers) {
            if (p->exists(path)) {
                return p->open(path);
            }
        }

        ::logger.error("Failed to open file `{}`", path);
        return nullptr;
    }

} // namespace tavros::resources

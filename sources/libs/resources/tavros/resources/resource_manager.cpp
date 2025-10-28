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

    core::shared_ptr<resource> resource_manager::open(core::string_view path, resource_access access)
    {
        for (auto& p : m_providers) {
            switch (access) {
            case resource_access::read_only:
                if (p->available_for_read(path) && p->exists(path)) {
                    return p->open(path);
                }
                break;

            case resource_access::write_only:
                if (p->available_for_write(path)) {
                    return p->open(path);
                }
                break;

            case resource_access::read_write:
                if (p->available_for_read(path) && p->available_for_write(path)) {
                    return p->open(path);
                }
                break;
            }
        }

        ::logger.error("Failed to open file `{}`", path);
        return nullptr;
    }

} // namespace tavros::resources

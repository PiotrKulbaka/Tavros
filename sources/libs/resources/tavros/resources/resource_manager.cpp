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

    void resource_manager::mount(core::shared_ptr<resource_provider> provider)
    {
        for (auto& p : m_providers) {
            if (p == provider) {
                return;
            }
        }
        m_providers.push_back(provider);
    }

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

    size_t resource_manager::read_data(core::string_view path, core::resizable_buffer<uint8>& buffer)
    {
        auto res = open(path);
        if (!res) {
            return 0;
        }
        auto* reader = res->reader();
        if (reader) {
            auto size = reader->size();
            buffer.ensure_size(size);
            return reader->read(buffer.data(), size);
        }

        ::logger.error("Failed to read data from file `{}`", path);
        return 0;
    }

} // namespace tavros::resources

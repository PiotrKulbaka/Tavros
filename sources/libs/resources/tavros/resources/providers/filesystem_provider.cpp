#include <tavros/resources/providers/filesystem_provider.hpp>

#include <tavros/resources/io/file_resource.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>

#include <filesystem>

namespace
{
    tavros::core::logger logger("filesystem_provider");
}
namespace tavros::resources
{

    filesystem_provider::filesystem_provider(core::string_view path, resource_access access)
        : m_base(path)
        , m_access(access)
    {
        if (!std::filesystem::exists(path)) {
            ::logger.warning("Filesystem provider for `{}` created, but path is invalid", path);
        }
    }

    filesystem_provider::~filesystem_provider() = default;

    bool filesystem_provider::available_for_read(core::string_view path) const
    {
        return resource_access::read_only == m_access || resource_access::read_write == m_access;
    }

    bool filesystem_provider::available_for_write(core::string_view path) const
    {
        return resource_access::write_only == m_access || resource_access::read_write == m_access;
    }

    bool filesystem_provider::exists(core::string_view path) const
    {
        try {
            std::filesystem::path full_path = std::filesystem::path(m_base) / std::filesystem::path(path);
            return std::filesystem::exists(full_path);
        } catch (const std::exception& e) {
            ::logger.error("exists() failed for '{}': {}", path, e.what());
            return false;
        }
    }

    core::shared_ptr<resource> filesystem_provider::open(core::string_view path)
    {
        std::filesystem::path full_path = std::filesystem::path(m_base) / std::filesystem::path(path);
        auto                  native_path = full_path.string();
        try {
            auto res = core::make_shared<file_resource>(native_path, m_access);
            return res;
        } catch (const std::exception& e) {
            logger.error("Exception while opening '{}': {}", native_path, e.what());
            TAV_DEBUG_BREAK();
            return nullptr;
        }
    }

} // namespace tavros::resources
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

    filesystem_provider::filesystem_provider(core::string_view path)
        : m_base(path)
    {
    }

    filesystem_provider::~filesystem_provider() = default;

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
        try {
            auto res = core::make_shared<file_resource>(full_path.string());
            return res;
        } catch (const std::exception& e) {
            logger.error("Exception while opening '{}': {}", full_path.string(), e.what());
            TAV_DEBUG_BREAK();
            return nullptr;
        }
    }

} // namespace tavros::resources
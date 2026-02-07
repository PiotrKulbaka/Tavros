#include <tavros/resources/providers/filesystem_provider.hpp>

#include <tavros/resources/io/file_resource.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/exception.hpp>

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
        if (!exists(path)) {
            ::logger.warning("Filesystem provider for `{}` created, but path is invalid", path);
        }
    }

    filesystem_provider::~filesystem_provider() = default;

    bool filesystem_provider::available_for_read(core::string_view path) const noexcept
    {
        return resource_access::read_only == m_access || resource_access::read_write == m_access;
    }

    bool filesystem_provider::available_for_write(core::string_view path) const noexcept
    {
        return resource_access::write_only == m_access || resource_access::read_write == m_access;
    }

    bool filesystem_provider::exists(core::string_view path) const
    {
        std::filesystem::path full_path = std::filesystem::path(m_base) / std::filesystem::path(path);
        std::error_code       ec;
        auto                  ret = std::filesystem::exists(full_path, ec);
        if (ec.value() == 0) {
            return ret;
        }

        core::file_error_tag et = core::file_error_tag::other;

        switch (ec.value()) {
        case static_cast<int>(std::errc::permission_denied):
            et = core::file_error_tag::permission_denied;
            break;
        case static_cast<int>(std::errc::no_such_file_or_directory):
            et = core::file_error_tag::not_found;
            break;
        case static_cast<int>(std::errc::filename_too_long):
            et = core::file_error_tag::invalid_path;
            break;
        default:
            break;
        }

        throw core::file_error(core::file_error_tag::permission_denied, full_path.string(), ec.message());
    }

    core::shared_ptr<resource> filesystem_provider::open(core::string_view path)
    {
        auto full_path = std::filesystem::path(m_base) / std::filesystem::path(path);
        auto native_path = full_path.string();
        return core::make_shared<file_resource>(native_path, m_access);
    }

} // namespace tavros::resources
#include <tavros/assets/providers/filesystem_provider.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/exception.hpp>
#include <tavros/assets/io/file_stream.hpp>

#include <filesystem>

namespace
{
    tavros::core::logger logger("filesystem_provider");
}

namespace tavros::assets
{

    filesystem_provider::filesystem_provider(core::string_view path, asset_open_mode open_mode)
        : m_base(path)
        , m_open_mode(open_mode)
    {
        if (!exists(path)) {
            ::logger.error("Filesystem provider for `{}` created, but path is invalid", path);
        }
    }

    filesystem_provider::~filesystem_provider() = default;

    bool filesystem_provider::can_read(core::string_view path) const noexcept
    {
        return asset_open_mode::read_only == m_open_mode;
    }

    bool filesystem_provider::can_write(core::string_view path) const noexcept
    {
        return asset_open_mode::write_only == m_open_mode;
    }

    bool filesystem_provider::exists(core::string_view path) const
    {
        auto            full_path = std::filesystem::path(m_base) / std::filesystem::path(path);
        std::error_code ec;
        auto            ret = std::filesystem::exists(full_path, ec);
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

    core::unique_ptr<asset_stream> filesystem_provider::open(core::string_view path, asset_open_mode open_mode)
    {
        auto full_path = std::filesystem::path(m_base) / std::filesystem::path(path);
        if (open_mode != m_open_mode) {
            throw core::file_error(core::file_error_tag::open_failed, full_path.string(), "incorrect opening mode");
        }
        auto native_path = full_path.string();
        return core::make_unique<file_stream>(native_path, open_mode);
    }

} // namespace tavros::assets
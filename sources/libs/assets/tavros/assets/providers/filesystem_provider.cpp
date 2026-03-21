#include <tavros/assets/providers/filesystem_provider.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/exception.hpp>
#include <tavros/assets/io/file_stream.hpp>


namespace
{
    tavros::core::logger logger("filesystem_provider");
}

namespace tavros::assets
{

    filesystem_provider::filesystem_provider(core::string_view path, core::string_view scheme, asset_open_mode open_mode)
        : m_scheme(scheme)
        , m_open_mode(open_mode)
    {
        filesystem::normalize_path(path, m_base);

        if (!filesystem::exists(m_base)) {
            ::logger.error("Filesystem provider for `{}` created, but path is invalid", path);
        }
    }

    filesystem_provider::~filesystem_provider() = default;

    core::string_view filesystem_provider::scheme() const noexcept
    {
        return m_scheme;
    }

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
        filesystem::fixed_path full_path = m_base;
        full_path /= path;
        return filesystem::exists(full_path);
    }

    core::unique_ptr<asset_stream> filesystem_provider::open(core::string_view path, asset_open_mode open_mode)
    {
        filesystem::fixed_path full_path = m_base;
        full_path /= path;
        if (open_mode != m_open_mode) {
            throw core::file_error(core::file_error_tag::open_failed, full_path, "incorrect opening mode");
        }
        return core::make_unique<file_stream>(full_path, open_mode);
    }

} // namespace tavros::assets
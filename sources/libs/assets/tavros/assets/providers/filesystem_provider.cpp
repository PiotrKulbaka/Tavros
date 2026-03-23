#include <tavros/assets/providers/filesystem_provider.hpp>

#include <tavros/core/io/file_reader.hpp>
#include <tavros/core/io/file_writer.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/exception.hpp>


namespace
{
    tavros::core::logger logger("filesystem_provider");
}

namespace tavros::assets
{

    filesystem_provider::filesystem_provider(core::string_view path, core::string_view scheme, bool for_read, bool for_write)
        : m_scheme(scheme)
        , m_can_read(for_read)
        , m_can_write(for_write)
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
        TAV_UNUSED(path);
        return m_can_read;
    }

    bool filesystem_provider::can_write(core::string_view path) const noexcept
    {
        TAV_UNUSED(path);
        return m_can_write;
    }

    bool filesystem_provider::exists(core::string_view path) const
    {
        filesystem::fixed_path full_path = m_base;
        full_path /= path;
        return filesystem::exists(full_path);
    }

    core::unique_ptr<core::basic_stream_reader> filesystem_provider::open_reader(core::string_view path)
    {
        filesystem::fixed_path full_path = m_base;
        full_path /= path;
        if (!m_can_read) {
            throw core::file_error(core::file_error_tag::open_failed, full_path, "unavailable for reading");
        }
        return core::make_unique<core::file_reader>(full_path);
    }

    core::unique_ptr<core::basic_stream_writer> filesystem_provider::open_writer(core::string_view path)
    {
        filesystem::fixed_path full_path = m_base;
        full_path /= path;
        if (!m_can_write) {
            throw core::file_error(core::file_error_tag::open_failed, full_path, "unavailable for writing");
        }
        return core::make_unique<core::file_writer>(full_path, core::file_open_mode::truncate);
    }

} // namespace tavros::assets
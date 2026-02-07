#include <tavros/resources/io/file_resource.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/exception.hpp>

namespace
{
    tavros::core::logger logger("file_resource");
}

namespace tavros::resources
{

    file_resource::file_resource(core::string_view path, resource_access access)
        : m_path(path)
        , m_access(access)
    {
        switch (m_access) {
        case resource_access::read_only:
            m_reader.open(m_path);
            if (!m_reader.is_open()) {
                throw core::file_error(core::file_error_tag::read_error, m_path, "Failed to open file for reading");
            }
            break;
        case resource_access::write_only:
            m_writer.open(m_path);
            if (!m_writer.is_open()) {
                throw core::file_error(core::file_error_tag::write_error, m_path, "Failed to open file for writing");
            }
            break;
        case resource_access::read_write:
            throw core::file_error(core::file_error_tag::invalid_argument, m_path, "resource_access::read_write is not supported yet");
        default:
            TAV_UNREACHABLE();
            break;
        }
    }

    file_resource::~file_resource()
    {
        close();
    }

    core::shared_ptr<resource_reader> file_resource::reader()
    {
        if (resource_access::read_only != m_access && resource_access::read_write != m_access) {
            throw core::file_error(core::file_error_tag::read_error, m_path, "Attempt to open reader, but no read access");
        }

        if (!m_reader.is_open()) {
            throw core::file_error(core::file_error_tag::read_error, m_path, "Reader is closed");
        }

        auto self = shared_from_this();
        return core::shared_ptr<resource_reader>(self, static_cast<resource_reader*>(&m_reader));
    }

    core::shared_ptr<resource_writer> file_resource::writer()
    {
        if (resource_access::write_only != m_access && resource_access::read_write != m_access) {
            throw core::file_error(core::file_error_tag::read_error, m_path, "Attempt to open reader, but no write access");
        }

        if (!m_writer.is_open()) {
            throw core::file_error(core::file_error_tag::read_error, m_path, "Writer is closed");
        }

        auto self = shared_from_this();
        return core::shared_ptr<resource_writer>(self, &m_writer);
    }

    core::string_view file_resource::path() const noexcept
    {
        return m_path;
    }

    bool file_resource::is_open() const noexcept
    {
        return m_reader.is_open() || m_writer.is_open();
    }

    void file_resource::close()
    {
        if (m_reader.is_open()) {
            m_reader.close();
        }

        if (m_writer.is_open()) {
            m_writer.close();
        }
    }

} // namespace tavros::resources

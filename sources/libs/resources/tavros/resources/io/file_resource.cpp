#include <tavros/resources/io/file_resource.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>

namespace
{
    tavros::core::logger logger("file_resource");
}

namespace tavros::resources
{

    file_resource::file_resource(core::string_view path)
        : m_path(path)
    {
    }

    file_resource::~file_resource()
    {
        close();
    }

    resource_reader* file_resource::reader()
    {
        if (m_writer.is_open()) {
            ::logger.error("Attempt to open reader while writer is active for '{}'", m_path);
            TAV_DEBUG_BREAK();
            return nullptr;
        }

        if (!m_reader.is_open()) {
            m_reader.open(m_path);
            if (!m_reader.is_open()) {
                ::logger.error("Failed to open reader for '{}'", m_path);
                TAV_DEBUG_BREAK();
                return nullptr;
            }
        }

        return &m_reader;
    }

    resource_writer* file_resource::writer()
    {
        if (m_reader.is_open()) {
            ::logger.error("Attempt to open writer while reader is active for '{}'", m_path);
            TAV_DEBUG_BREAK();
            return nullptr;
        }

        if (!m_writer.is_open()) {
            m_writer.open(m_path);
            if (!m_writer.is_open()) {
                ::logger.error("Failed to open writer for '{}'", m_path);
                TAV_DEBUG_BREAK();
                return nullptr;
            }
        }

        return &m_writer;
    }

    core::string_view file_resource::path() const
    {
        return m_path;
    }

    bool file_resource::is_open() const
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

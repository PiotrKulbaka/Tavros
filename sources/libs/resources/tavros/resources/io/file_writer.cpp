#include <tavros/resources/io/file_writer.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <string>

namespace
{
    tavros::core::logger logger("file_writer");

    const char* seek_dir_to_str(tavros::resources::seek_dir dir) noexcept
    {
        switch (dir) {
        case tavros::resources::seek_dir::begin:
            return "begin";
        case tavros::resources::seek_dir::current:
            return "current";
        case tavros::resources::seek_dir::end:
            return "end";
        default:
            TAV_UNREACHABLE();
            return "";
        }
    }
} // namespace

namespace tavros::resources
{

    file_writer::file_writer() = default;

    file_writer::~file_writer()
    {
        if (m_file.is_open()) {
            close();
        }
    }

    void file_writer::open(core::string_view path, bool append)
    {
        try {
            // TODO: remake to path_view
            core::string str_path(path);
            auto         open_flags = std::ios::out | std::ios::binary;
            if (append) {
                open_flags |= std::ios::app;
            }

            m_file.open(str_path, open_flags);
            if (!m_file.is_open()) {
                ::logger.error("Failed to open file for writing: {}", str_path);
                TAV_DEBUG_BREAK();
                m_size = 0;
                return;
            }

            m_size = append ? static_cast<size_t>(m_file.tellp()) : 0;
        } catch (const std::exception& e) {
            ::logger.error("Exception while opening file for writing: {}", e.what());
            TAV_DEBUG_BREAK();
            m_size = 0;
        } catch (...) {
            ::logger.error("Unknown exception while opening file for writing");
            TAV_DEBUG_BREAK();
            m_size = 0;
        }
    }

    void file_writer::close()
    {
        m_file.close();
    }

    bool file_writer::is_open() const
    {
        return m_file.is_open();
    }

    size_t file_writer::write(core::buffer_view<uint8> buffer)
    {
        if (!m_file.is_open()) {
            ::logger.warning("Attempted to write to a file that is not open");
            TAV_DEBUG_BREAK();
            return 0;
        }

        if (buffer.empty()) {
            ::logger.warning("Write called with null buffer or size 0");
            return 0;
        }

        m_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        if (!m_file.good()) {
            ::logger.error("Failed to write {} bytes to file", buffer.size());
            TAV_DEBUG_BREAK();
            return 0;
        }

        auto pos = static_cast<size_t>(m_file.tellp());
        if (m_size < pos) {
            m_size = pos;
        }

        return buffer.size();
    }

    bool file_writer::seek(size_t offset, seek_dir dir)
    {
        if (!m_file.is_open()) {
            ::logger.warning("Attempted to seek in a file that is not open");
            TAV_DEBUG_BREAK();
            return false;
        }

        std::ios::seekdir sd;
        switch (dir) {
        case seek_dir::begin:
            sd = std::ios::beg;
            break;
        case seek_dir::current:
            sd = std::ios::cur;
            break;
        case seek_dir::end:
            sd = std::ios::end;
            break;
        default:
            TAV_UNREACHABLE();
            return false;
        }

        // Clear error flags before seek
        m_file.clear();
        m_file.seekp(static_cast<std::streamoff>(offset), sd);

        if (!m_file.good()) {
            ::logger.error("Failed to seek to position `{}` {}", seek_dir_to_str(dir), offset);
            TAV_DEBUG_BREAK();
            return false;
        }

        return true;
    }

    size_t file_writer::tell() const
    {
        if (!m_file.is_open()) {
            ::logger.warning("Attempted to tell position in a file that is not open");
            return 0;
        }

        auto pos = m_file.tellp();
        if (pos == -1) {
            ::logger.error("tellp() failed");
            TAV_DEBUG_BREAK();
            return 0;
        }

        return static_cast<size_t>(pos);
    }

    size_t file_writer::size() const
    {
        return m_size;
    }

    void file_writer::flush()
    {
        if (!m_file.is_open()) {
            ::logger.warning("Attempted to flush a file that is not open");
            return;
        }

        m_file.flush();
        if (!m_file.good()) {
            ::logger.error("Failed to flush file");
            TAV_DEBUG_BREAK();
        }
    }

    bool file_writer::good() const
    {
        return m_file.is_open() && m_file.good();
    }

} // namespace tavros::resources
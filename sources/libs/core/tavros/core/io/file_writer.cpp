#include <tavros/core/io/file_writer.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/exception.hpp>

namespace
{
    tavros::core::logger logger("file_writer");
} // namespace

namespace tavros::core
{
    file_writer::file_writer(string_view path)
        : m_size(0)
    {
        m_file.open(path.data(), std::ios::out | std::ios::binary);

        if (!m_file.is_open()) {
            throw file_error(file_error_tag::open_failed, path, "failed to open file for writing");
        }

        // Determine initial file size if file already existed
        m_file.seekp(0, std::ios::end);
        auto end_pos = m_file.tellp();
        m_size = (end_pos == std::streampos(-1)) ? 0 : static_cast<size_t>(end_pos);
        m_file.seekp(0, std::ios::beg);

        if (!m_file.good()) {
            throw file_error(file_error_tag::other, path, "file opened but stream is in bad state");
        }
    }

    size_t file_writer::write(const uint8* src, size_t size)
    {
        if (size == 0) {
            ::logger.warning("Write called with empty size");
            TAV_DEBUG_BREAK();
            return 0;
        }

        if (!m_file.good()) {
            set_state(stream_state::bad);
            return 0;
        }

        auto before = m_file.tellp();

        try {
            m_file.write(reinterpret_cast<const char*>(src), static_cast<std::streamsize>(size));
        } catch (const std::exception& e) {
            throw file_error(file_error_tag::write_error, "", e.what());
        } catch (...) {
            throw file_error(file_error_tag::write_error, "", "exception occurred during write");
        }

        if (!m_file.good()) {
            set_state(stream_state::bad);
            throw file_error(file_error_tag::write_error, "", "write failure");
        }

        auto after = m_file.tellp();
        auto written = static_cast<size_t>(after - before);

        // Track the high-water mark as the file size
        if (after != std::streampos(-1)) {
            m_size = std::max(m_size, static_cast<size_t>(after));
        }

        if (written != size) {
            set_state(stream_state::bad);
            throw file_error(file_error_tag::write_error, "", "partial write");
        }

        return written;
    }

    bool file_writer::seekable() const noexcept
    {
        return true;
    }

    bool file_writer::seek(ssize_t offset, seek_dir dir) noexcept
    {
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
        }

        try {
            m_file.seekp(static_cast<std::streamoff>(offset), sd);

            if (!m_file.good()) {
                ::logger.error("Failed to seek to position `{}` {}", to_string(dir), offset);
                m_file.clear();
                TAV_DEBUG_BREAK();
                return false;
            }

            return true;
        } catch (...) {
            ::logger.warning("Exception occurred during seek()");
            m_file.clear();
            TAV_DEBUG_BREAK();
            return false;
        }
    }

    ssize_t file_writer::tell() const noexcept
    {
        try {
            auto pos = m_file.tellp();
            if (pos == std::streampos(-1)) {
                return -1;
            }
            return static_cast<ssize_t>(pos);
        } catch (...) {
            ::logger.warning("Exception occurred during tell()");
            TAV_DEBUG_BREAK();
            return -1;
        }
    }

    size_t file_writer::size() const noexcept
    {
        return m_size;
    }

} // namespace tavros::core

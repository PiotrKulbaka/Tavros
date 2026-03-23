#include <tavros/core/io/file_reader.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/exception.hpp>

namespace
{
    tavros::core::logger logger("file_reader");
} // namespace

namespace tavros::core
{
    file_reader::file_reader(string_view path)
        : m_size(0)
    {
        m_file.open(path.data(), std::ios::in | std::ios::binary);

        if (!m_file.is_open()) {
            throw file_error(file_error_tag::open_failed, path, "failed to open file for reading");
        }

        m_file.seekg(0, std::ios::end);
        m_size = static_cast<size_t>(m_file.tellg());
        m_file.seekg(0, std::ios::beg);

        if (!m_file.good()) {
            throw file_error(file_error_tag::other, path, "file opened but stream is in bad state");
        }
    }

    size_t file_reader::read(uint8* dst, size_t size)
    {
        if (size == 0) {
            ::logger.warning("Read called with empty size");
            return 0;
        }

        if (!good()) {
            // Already in bad state
            return 0;
        }

        if (!m_file.good()) {
            set_state(m_file.eof() ? stream_state::eos : stream_state::bad);
            return 0;
        }

        try {
            m_file.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(size));
        } catch (const std::exception& e) {
            throw file_error(file_error_tag::read_error, "", e.what());
        } catch (...) {
            throw file_error(file_error_tag::read_error, "", "exception occurred during read");
        }

        auto bytes_read = static_cast<size_t>(m_file.gcount());
        if (bytes_read < size) {
            if (m_file.eof()) {
                set_state(stream_state::eos);
            } else {
                set_state(stream_state::bad);
                throw file_error(file_error_tag::read_error, "", "read failure");
            }
        }

        return bytes_read;
    }

    bool file_reader::seekable() const noexcept
    {
        return true;
    }

    bool file_reader::seek(ssize_t offset, seek_dir dir) noexcept
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
            const bool was_eos = eos() && m_file.eof();
            if (was_eos) {
                m_file.clear();
            }

            m_file.seekg(static_cast<std::streamoff>(offset), sd);

            if (!m_file.good()) {
                ::logger.error("Failed to seek to position `{}` {}", to_string(dir), offset);
                return false;
            }

            // Check if new position is within readable range
            if (was_eos) {
                const auto pos = static_cast<size_t>(m_file.tellg());
                if (pos < m_size) {
                    set_state(stream_state::good);
                }
            }

            return true;
        } catch (...) {
            ::logger.warning("Exception occurred during seek()");
            TAV_DEBUG_BREAK();
            return false;
        }
    }

    ssize_t file_reader::tell() const noexcept
    {
        try {
            auto pos = m_file.tellg();
            if (pos == std::streampos(-1)) {
                return -1;
            }
            return static_cast<ssize_t>(pos);
        } catch (...) {
            ::logger.warning("Exception occurred during tell()");
            TAV_DEBUG_BREAK();
            return static_cast<ssize_t>(-1);
        }
    }

    size_t file_reader::size() const noexcept
    {
        return m_size;
    }

} // namespace tavros::core
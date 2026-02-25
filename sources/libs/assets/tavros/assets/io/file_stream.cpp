#include <tavros/assets/io/file_stream.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/exception.hpp>
#include <string>

namespace
{
    tavros::core::logger logger("file_stream");

    const char* seek_dir_to_str(tavros::assets::seek_dir dir) noexcept
    {
        switch (dir) {
        case tavros::assets::seek_dir::begin:
            return "begin";
        case tavros::assets::seek_dir::current:
            return "current";
        case tavros::assets::seek_dir::end:
            return "end";
        default:
            TAV_UNREACHABLE();
            return "";
        }
    }
} // namespace

namespace tavros::assets
{

    file_stream::file_stream(core::string_view path, asset_open_mode open_mode)
        : m_size(0)
        , m_path(path)
        , m_open_mode(open_mode)
    {
        std::ios_base::openmode om;
        switch (m_open_mode) {
        case asset_open_mode::read_only:
            om = std::ios::in | std::ios::binary;
            break;
        case asset_open_mode::write_only:
            om = std::ios::out | std::ios::binary;
            break;
        }

        m_file.open(m_path, om);

        if (!m_file.is_open()) {
            throw core::file_error(core::file_error_tag::open_failed, m_path, "failed to open file for reading");
        }

        if (asset_open_mode::read_only == m_open_mode) {
            m_file.seekg(0, std::ios::end);
            m_size = static_cast<size_t>(m_file.tellg());
            m_file.seekg(0, std::ios::beg);
        }

        if (!m_file.good()) {
            throw core::file_error(core::file_error_tag::other, m_path, "file opened but stream is in bad state");
        }
    }

    file_stream::~file_stream() noexcept
    {
        if (m_file.is_open()) {
            m_file.close();
        }
    }

    size_t file_stream::read(core::buffer_span<uint8> buffer)
    {
        if (buffer.empty()) {
            ::logger.warning("Read called with empty buffer");
            TAV_DEBUG_BREAK();
            return 0;
        }

        if (asset_open_mode::read_only != m_open_mode) {
            throw core::file_error(core::file_error_tag::read_error, m_path, "attempting to read a file opened for writing");
        }

        try {
            m_file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            if (m_file.bad()) {
                m_file.clear();
                throw core::file_error(core::file_error_tag::read_error, m_path, "file is in bad condition after reading");
            }
            return static_cast<size_t>(m_file.gcount());
        } catch (const std::exception& e) {
            throw core::file_error(core::file_error_tag::read_error, m_path, e.what());
        } catch (...) {
            throw core::file_error(core::file_error_tag::read_error, m_path, "exception occurred during read");
        }
    }

    size_t file_stream::write(core::buffer_view<uint8> buffer)
    {
        if (buffer.empty()) {
            ::logger.warning("Write called with empty buffer");
            return 0;
        }

        if (asset_open_mode::write_only != m_open_mode) {
            throw core::file_error(core::file_error_tag::write_error, m_path, "attempting to write a file opened for reading");
        }

        try {
            m_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            if (m_file.bad()) {
                m_file.clear();
                throw core::file_error(core::file_error_tag::write_error, m_path, "file is in bad condition after writing");
            }

            auto pos = static_cast<size_t>(m_file.tellp());
            if (m_size < pos) {
                m_size = pos;
            }

            return buffer.size();
        } catch (const std::exception& e) {
            throw core::file_error(core::file_error_tag::write_error, m_path, e.what());
        } catch (...) {
            throw core::file_error(core::file_error_tag::write_error, m_path, "exception occurred during write");
        }
    }

    bool file_stream::seek(ssize_t offset, seek_dir dir) noexcept
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
            // Clear error flags before seek
            m_file.seekg(static_cast<std::streamoff>(offset), sd);

            if (!m_file.good()) {
                ::logger.error("Failed to seek to position `{}` {}, in file '{}'", seek_dir_to_str(dir), offset, m_path);
                m_file.clear();
                TAV_DEBUG_BREAK();
                return false;
            }

            return true;
        } catch (...) {
            ::logger.warning("Exception occurred during seek() in file '{}'", m_path);
            m_file.clear();
            TAV_DEBUG_BREAK();
            return false;
        }
    }

    ssize_t file_stream::tell() const noexcept
    {
        try {
            auto pos = m_file.tellg();
            return static_cast<ssize_t>(pos);
        } catch (...) {
            ::logger.warning("Exception occurred during tell() in file '{}'", m_path);
            m_file.clear();
            TAV_DEBUG_BREAK();
            return static_cast<ssize_t>(-1);
        }
    }

    size_t file_stream::size() const noexcept
    {
        return m_size;
    }

    bool file_stream::eos() const noexcept
    {
        return m_file.eof();
    }

} // namespace tavros::assets

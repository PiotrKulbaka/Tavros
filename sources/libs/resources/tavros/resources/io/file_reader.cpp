#include <tavros/resources/io/file_reader.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <string>

namespace
{
    tavros::core::logger logger("file_reader");

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

    file_reader::file_reader() = default;

    file_reader::~file_reader()
    {
        if (m_file.is_open()) {
            close();
        }
    }

    void file_reader::open(core::string_view path)
    {
        try {
            // TODO: remake to path_view
            core::string str_path(path);
            m_file.open(str_path, std::ios::in | std::ios::binary);
            if (!m_file.is_open()) {
                ::logger.error("Failed to open file for reading: {}", str_path);
                TAV_DEBUG_BREAK();
                m_size = 0;
                return;
            }

            m_file.seekg(0, std::ios::end);
            m_size = static_cast<size_t>(m_file.tellg());
            m_file.seekg(0, std::ios::beg);

            if (!m_file.good()) {
                ::logger.warning("File opened but stream is in bad state: {}", str_path);
            }
        } catch (const std::exception& e) {
            ::logger.error("Exception while opening file for reading: {}", e.what());
            TAV_DEBUG_BREAK();
            m_size = 0;
        } catch (...) {
            ::logger.error("Unknown exception while opening file for reading");
            TAV_DEBUG_BREAK();
            m_size = 0;
        }
    }

    void file_reader::close()
    {
        m_file.close();
    }

    bool file_reader::is_open() const
    {
        return m_file.is_open();
    }

    size_t file_reader::read(core::buffer_span<uint8> buffer)
    {
        if (buffer.empty()) {
            ::logger.warning("Read called with null buffer or size 0");
            TAV_DEBUG_BREAK();
            return 0;
        }

        try {
            m_file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            if (m_file.bad()) {
                ::logger.error("Read failed");
                TAV_DEBUG_BREAK();
                return 0;
            }
            return static_cast<size_t>(m_file.gcount());
        } catch (const std::exception& e) {
            ::logger.error("Exception during read: {}", e.what());
            TAV_DEBUG_BREAK();
            return 0;
        } catch (...) {
            ::logger.error("Unknown exception during read");
            TAV_DEBUG_BREAK();
            return 0;
        }
    }

    bool file_reader::seek(size_t offset, seek_dir dir)
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
        }

        // Clear error flags before seek
        m_file.clear();
        m_file.seekg(static_cast<std::streamoff>(offset), sd);

        if (!m_file.good()) {
            ::logger.error("Failed to seek to position `{}` {}", seek_dir_to_str(dir), offset);
            TAV_DEBUG_BREAK();
            return false;
        }

        return true;
    }

    size_t file_reader::tell() const
    {
        if (!m_file.is_open()) {
            ::logger.warning("Attempted to tell position in a file that is not open");
            return 0;
        }

        auto pos = m_file.tellg();
        if (pos == -1) {
            ::logger.error("tellg() failed");
            TAV_DEBUG_BREAK();
            return 0;
        }
        return static_cast<size_t>(pos);
    }

    size_t file_reader::size() const
    {
        return m_size;
    }

    bool file_reader::eos() const
    {
        if (!m_file.is_open()) {
            return true;
        }
        return m_file.eof();
    }

    bool file_reader::good() const
    {
        return m_file.is_open() && m_file.good();
    }

    file_reader::content_result file_reader::read_content() const
    {
        if (!good()) {
            ::logger.error("File in bad state");
            TAV_DEBUG_BREAK();
            return {};
        }

        // Save current position
        const auto current_pos = m_file.tellg();
        if (current_pos < 0) {
            ::logger.error("tellg() failed in read_content()");
            TAV_DEBUG_BREAK();
            return {};
        }

        // Move to beginning
        m_file.seekg(0, std::ios::beg);
        if (!m_file.good()) {
            m_file.seekg(current_pos);
            ::logger.error("tellg() failed in read_content()");
            return {};
        }

        // Read whole content
        try {
            core::string data;
            data.resize(m_size);
            m_file.read(data.data(), static_cast<std::streamsize>(m_size));

            if (m_file.bad()) {
                ::logger.error("Read content failed");
                m_file.clear();
                m_file.seekg(current_pos);
                TAV_DEBUG_BREAK();
                return {};
            }

            m_file.clear();
            m_file.seekg(current_pos);

            return {true, data};

        } catch (const std::exception& e) {
            ::logger.error("Exception during read content: {}", e.what());
            TAV_DEBUG_BREAK();
            return {};
        } catch (...) {
            ::logger.error("Unknown exception during read content");
            TAV_DEBUG_BREAK();
            return {};
        }
    }

} // namespace tavros::resources

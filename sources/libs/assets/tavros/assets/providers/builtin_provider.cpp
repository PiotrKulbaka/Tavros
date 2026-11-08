#include <tavros/assets/providers/builtin_provider.hpp>

#include <tavros/core/compression/compression.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/debug_break.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/exception.hpp>

#include <tavros/assets/builtin/fonts/consola_mono_ttf.hpp>

namespace
{
    tavros::core::logger logger("filesystem_provider");

    class memory_reader final : public tavros::core::basic_stream_reader
    {
    public:
        explicit memory_reader(tavros::core::dynamic_buffer<uint8> data) noexcept
            : m_data(std::move(data))
        {
        }

        ~memory_reader() noexcept override = default;

        size_t read(uint8* dst, size_t size) override
        {
            TAV_ASSERT(dst);

            const size_t available = m_data.size_bytes() - m_cursor;
            const size_t to_read = std::min(size, available);

            m_data.copy_to(dst, to_read, m_cursor);
            m_cursor += to_read;

            return to_read;
        }

        bool seekable() const noexcept override
        {
            return true;
        }

        bool seek(ssize_t offset, tavros::core::seek_dir dir) noexcept override
        {
            ssize_t new_pos = 0;

            switch (dir) {
            case tavros::core::seek_dir::begin:
                new_pos = offset;
                break;

            case tavros::core::seek_dir::current:
                new_pos = static_cast<ssize_t>(m_cursor) + offset;
                break;

            case tavros::core::seek_dir::end:
                new_pos = static_cast<ssize_t>(m_data.size_bytes()) + offset;
                break;

            default:
                return false;
            }

            if (new_pos < 0 || new_pos > static_cast<ssize_t>(m_data.size_bytes())) {
                return false;
            }

            m_cursor = static_cast<size_t>(new_pos);
            return true;
        }

        ssize_t tell() const noexcept override
        {
            return static_cast<ssize_t>(m_cursor);
        }

        size_t size() const noexcept override
        {
            return m_data.size_bytes();
        }

    private:
        tavros::core::dynamic_buffer<uint8> m_data;
        size_t                              m_cursor = 0;
    };

} // namespace

namespace tavros::assets
{

    builtin_provider::builtin_provider() noexcept
    {
        register_asset(
            "fonts/Consola-Mono.ttf",
            k_consola_mono_ttf_compressed_data,
            k_consola_mono_ttf_compressed_size,
            k_consola_mono_ttf_uncompressed_size,
            true
        );
    }

    builtin_provider::~builtin_provider() = default;

    core::string_view builtin_provider::scheme() const noexcept
    {
        return "builtin";
    }

    bool builtin_provider::can_read(core::string_view path) const noexcept
    {
        TAV_UNUSED(path);
        return true;
    }

    bool builtin_provider::can_write(core::string_view path) const noexcept
    {
        TAV_UNUSED(path);
        return false;
    }

    bool builtin_provider::exists(core::string_view path) const
    {
        return m_assets.find(path) != m_assets.end();
    }

    core::unique_ptr<core::basic_stream_reader> builtin_provider::open_reader(core::string_view path)
    {
        auto it = m_assets.find(path);
        if (it == m_assets.end()) {
            logger.error("Builtin asset '{}' not found", path);
            throw core::file_error(core::file_error_tag::open_failed, path, "Builtin asset not found");
        }

        const auto& entry = it->second;

        tavros::core::dynamic_buffer<uint8> data(entry.uncompressed_size);

        if (entry.compressed) {
            core::buffer_view<const uint8> compressed(entry.data, entry.compressed_size);
            core::buffer_view<uint8>       output(data.data(), data.size_bytes());

            if (!core::uncompress_data(compressed, output)) {
                logger.error("Failed to uncompress '{}' builtin asset.", path);
                throw core::file_error(core::file_error_tag::read_error, path, "Failed to uncompress builtin asset");
            }
        } else {
            TAV_ASSERT(entry.compressed_size == entry.uncompressed_size);
            data.copy_from(entry.data, entry.uncompressed_size);
        }

        return core::make_unique<memory_reader>(std::move(data));
    }

    core::unique_ptr<core::basic_stream_writer> builtin_provider::open_writer(core::string_view path)
    {
        throw core::file_error(core::file_error_tag::open_failed, path, "Builtin unavailable for writing");
    }

    void builtin_provider::register_asset(
        core::string_view path,
        const uint8*      data,
        size_t            compressed_size,
        size_t            uncompressed_size,
        bool              is_compressed
    )
    {
        if (m_assets.find(path) != m_assets.end()) {
            logger.error("Asset '{}' is already registered", path);
            return;
        }

        m_assets.emplace(core::string(path), asset_entry{data, compressed_size, uncompressed_size, is_compressed});
    }

} // namespace tavros::assets

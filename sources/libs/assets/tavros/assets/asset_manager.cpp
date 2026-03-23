#include <tavros/assets/asset_manager.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/exception.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/unreachable.hpp>

namespace
{
    tavros::core::logger logger("asset_manager");

    struct parsed_uri
    {
        tavros::core::string_view scheme;
        tavros::core::string_view path;
    };

    static parsed_uri parse_uri(tavros::core::string_view uri) noexcept
    {
        auto pos = uri.find("://");
        if (pos == tavros::core::string_view::npos) {
            return {"", uri}; // no scheme
        }
        return {uri.substr(0, pos), uri.substr(pos + 3)};
    }
} // namespace

namespace tavros::assets
{

    asset_manager::asset_manager() = default;

    asset_manager::~asset_manager() noexcept = default;

    bool asset_manager::exists(core::string_view path) const
    {
        for (auto& p : m_providers) {
            if (p->exists(path)) {
                return true;
            }
        }
        return false;
    }

    core::unique_ptr<core::basic_stream_reader> asset_manager::open_reader(core::string_view path) const
    {
        auto [scheme, only_path] = parse_uri(path);
        for (auto& p : m_providers) {
            if ((p->scheme().empty() && scheme.empty() || p->scheme() == scheme) && p->exists(only_path)) {
                return p->open_reader(only_path);
            }
        }
        throw core::file_error(core::file_error_tag::open_failed, path, "open_reader() failed");
    }

    core::unique_ptr<core::basic_stream_writer> asset_manager::open_writer(core::string_view path) const
    {
        auto [scheme, only_path] = parse_uri(path);
        for (auto& p : m_providers) {
            if (p->scheme().empty() && scheme.empty() || p->scheme() == scheme) {
                return p->open_writer(only_path);
            }
        }
        throw core::file_error(core::file_error_tag::open_failed, path, "open_writer() failed");
    }

    core::unique_ptr<core::basic_stream_reader> asset_manager::try_open_reader(core::string_view path) const noexcept
    {
        try {
            return open_reader(path);
        } catch (core::file_error& ex) {
            logger.error("An exception occurred while opening file reader '{}'", ex.what());
        } catch (std::exception& ex) {
            logger.error("An exception occurred while opening file reader '{}'", ex.what());
        } catch (...) {
            logger.error("An exception occurred while opening file reader");
        }
        return nullptr;
    }

    core::unique_ptr<core::basic_stream_writer> asset_manager::try_open_writer(core::string_view path) const noexcept
    {
        try {
            return open_writer(path);
        } catch (core::file_error& ex) {
            logger.error("An exception occurred while opening file writer '{}'", ex.what());
        } catch (std::exception& ex) {
            logger.error("An exception occurred while opening file writer '{}'", ex.what());
        } catch (...) {
            logger.error("An exception occurred while opening file writer");
        }
        return nullptr;
    }

    core::string asset_manager::read_text(core::string_view path) const
    {
        auto         rd = open_reader(path);
        core::string str(rd->size(), '\0');
        auto         rd_sz = rd->read(reinterpret_cast<uint8*>(str.data()), str.size());
        str.resize(rd_sz);
        return str;
    }

    core::dynamic_buffer<uint8> asset_manager::read_binary(core::string_view path) const
    {
        auto                        rd = open_reader(path);
        core::dynamic_buffer<uint8> bytes(rd->size());
        rd->read(bytes.data(), bytes.capacity());
        return bytes;
    }

} // namespace tavros::assets

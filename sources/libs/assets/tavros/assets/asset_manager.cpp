#include <tavros/assets/asset_manager.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/exception.hpp>
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

    core::unique_ptr<asset_stream> asset_manager::open(core::string_view path, asset_open_mode open_mode) const
    {
        auto [scheme, only_path] = parse_uri(path);

        switch (open_mode) {
        case asset_open_mode::read_only:
            for (auto& p : m_providers) {
                if ((p->scheme().empty() && scheme.empty() || p->scheme() == scheme) && p->exists(only_path)) {
                    return p->open(only_path, asset_open_mode::read_only);
                }
            }
            break;

        case asset_open_mode::write_only:
            for (auto& p : m_providers) {
                if (p->scheme().empty() && scheme.empty() || p->scheme() == scheme) {
                    return p->open(only_path, asset_open_mode::write_only);
                }
            }
            break;

        default:
            TAV_UNREACHABLE();
        }

        throw core::file_error(core::file_error_tag::invalid_path, path, "open() failed");
    }

    core::unique_ptr<asset_stream> asset_manager::try_open(core::string_view path, asset_open_mode open_mode) const noexcept
    {
        try {
            return open(path, open_mode);
        } catch (core::file_error& ex) {
            logger.error("An exception occurred while opening the file '{}'", ex.what());
        } catch (std::exception& ex) {
            logger.error("An exception occurred while opening the file '{}'", ex.what());
        } catch (...) {
            logger.error("An exception occurred while opening the file");
        }
        return nullptr;
    }

    core::string asset_manager::read_text(core::string_view path) const
    {
        auto stream = open(path, asset_open_mode::read_only);
        return stream->read_text();
    }

    core::dynamic_buffer<uint8> asset_manager::read_binary(core::string_view path) const
    {
        auto stream = open(path, asset_open_mode::read_only);
        return stream->read_binary();
    }

} // namespace tavros::assets

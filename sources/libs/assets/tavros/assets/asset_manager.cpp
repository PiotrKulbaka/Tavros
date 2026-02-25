#include <tavros/assets/asset_manager.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/exception.hpp>
#include <tavros/core/debug/unreachable.hpp>

namespace
{
    tavros::core::logger logger("asset_manager");
}

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
        for (auto& p : m_providers) {
            switch (open_mode) {
            case asset_open_mode::read_only:
                if (p->can_read(path) && p->exists(path)) {
                    return p->open(path, asset_open_mode::read_only);
                }
                break;

            case asset_open_mode::write_only:
                if (p->can_write(path)) {
                    return p->open(path, asset_open_mode::write_only);
                }
                break;

            default:
                TAV_UNREACHABLE();
            }
        }

        throw core::file_error(core::file_error_tag::invalid_path, path, "open() failed");
    }

    core::string asset_manager::read_text(core::string_view path) const
    {
        auto         stream = open(path, asset_open_mode::read_only);
        core::string content(stream->size(), '\0');
        auto         sz = stream->read({reinterpret_cast<uint8*>(content.data()), content.size()});
        content.resize(sz);
        return content;
    }

    core::vector<uint8> asset_manager::read_binary(core::string_view path) const
    {
        auto                stream = open(path, asset_open_mode::read_only);
        core::vector<uint8> content(stream->size(), 0);
        stream->read(content);
        return content;
    }

} // namespace tavros::assets

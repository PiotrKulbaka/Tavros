#pragma once

#include <tavros/core/containers/map.hpp>
#include <tavros/core/string.hpp>
#include <tavros/assets/asset_provider.hpp>
#include <tavros/core/utils/string_hash.hpp>

namespace tavros::assets
{

    class builtin_provider final : public asset_provider
    {
    public:
        builtin_provider() noexcept;

        ~builtin_provider() noexcept override;

        [[nodiscard]] core::string_view scheme() const noexcept override;

        [[nodiscard]] bool can_read(core::string_view path) const noexcept override;

        [[nodiscard]] bool can_write(core::string_view path) const noexcept override;

        [[nodiscard]] bool exists(core::string_view path) const override;

        [[nodiscard]] core::unique_ptr<core::basic_stream_reader> open_reader(core::string_view path) override;

        [[nodiscard]] core::unique_ptr<core::basic_stream_writer> open_writer(core::string_view path) override;

    private:
        struct asset_entry
        {
            const uint8* data;
            size_t       compressed_size;
            size_t       uncompressed_size;
            bool         compressed;
        };

        void register_asset(
            core::string_view path,
            const uint8*      data,
            size_t            compressed_size,
            size_t            uncompressed_size,
            bool              is_compressed
        );

    private:
        core::map<core::short_string, asset_entry, core::string_less> m_assets;
    };

} // namespace tavros::assets
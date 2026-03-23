#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/filesystem.hpp>
#include <tavros/assets/asset_provider.hpp>

namespace tavros::assets
{

    class filesystem_provider final : public asset_provider
    {
    public:
        filesystem_provider(core::string_view path, core::string_view scheme, bool for_read = true, bool for_write = false);

        ~filesystem_provider() noexcept override;

        [[nodiscard]] core::string_view scheme() const noexcept override;

        [[nodiscard]] bool can_read(core::string_view path) const noexcept override;

        [[nodiscard]] bool can_write(core::string_view path) const noexcept override;

        [[nodiscard]] bool exists(core::string_view path) const override;

        [[nodiscard]] core::unique_ptr<core::basic_stream_reader> open_reader(core::string_view path) override;

        [[nodiscard]] core::unique_ptr<core::basic_stream_writer> open_writer(core::string_view path) override;

    private:
        filesystem::fixed_path m_base;
        core::fixed_string<32> m_scheme;
        bool                   m_can_read;
        bool                   m_can_write;
    };

} // namespace tavros::assets
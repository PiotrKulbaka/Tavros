#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/filesystem.hpp>
#include <tavros/assets/asset_provider.hpp>
#include <tavros/assets/asset_open_mode.hpp>

namespace tavros::assets
{

    class filesystem_provider final : public asset_provider
    {
    public:
        filesystem_provider(core::string_view path, core::string_view scheme, asset_open_mode access);

        ~filesystem_provider() noexcept override;

        [[nodiscard]] core::string_view scheme() const noexcept override;

        [[nodiscard]] bool can_read(core::string_view path) const noexcept override;

        [[nodiscard]] bool can_write(core::string_view path) const noexcept override;

        [[nodiscard]] bool exists(core::string_view path) const override;

        [[nodiscard]] core::unique_ptr<asset_stream> open(core::string_view path, asset_open_mode open_mode) override;
    private:
        filesystem::fixed_path m_base;
        core::fixed_string<32> m_scheme;
        asset_open_mode        m_open_mode;
    };

} // namespace tavros::assets
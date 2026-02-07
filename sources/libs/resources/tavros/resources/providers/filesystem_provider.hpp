#pragma once

#include <tavros/resources/resource_provider.hpp>
#include <tavros/resources/resource_access.hpp>
#include <tavros/core/string.hpp>

namespace tavros::resources
{

    class filesystem_provider : public resource_provider
    {
    public:
        filesystem_provider(core::string_view path, resource_access access);

        ~filesystem_provider() noexcept override;

        [[nodiscard]] bool available_for_read(core::string_view path) const noexcept override;

        [[nodiscard]] bool available_for_write(core::string_view path) const noexcept override;

        [[nodiscard]] bool exists(core::string_view path) const override;

        [[nodiscard]] core::shared_ptr<resource> open(core::string_view path) override;

    private:
        core::string    m_base;
        resource_access m_access;
    };

} // namespace tavros::resources
#pragma once

#include <tavros/resources/resource_provider.hpp>
#include <tavros/core/string.hpp>

namespace tavros::resources
{

    class filesystem_provider : public resource_provider
    {
    public:
        filesystem_provider(core::string_view path);

        ~filesystem_provider() override;

        [[nodiscard]] bool exists(core::string_view path) const override;

        [[nodiscard]] core::shared_ptr<resource> open(core::string_view path) override;

    private:
        core::string m_base;
    };

} // namespace tavros::resources
#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/resources/resource.hpp>

namespace tavros::resources
{

    class resource_provider
    {
    public:
        virtual ~resource_provider() = default;

        virtual [[nodiscard]] bool exists(core::string_view path) const = 0;

        virtual [[nodiscard]] core::shared_ptr<resource> open(core::string_view path) = 0;
    };

} // namespace tavros::resources

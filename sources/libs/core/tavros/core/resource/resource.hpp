#pragma once

#include <tavros/core/types.hpp>

namespace tavros::core
{

    enum class resource_status : uint8
    {
        unloaded,
        pending,
        ready,
        failed,
    };

    struct resource_base
    {
        resource_status status = resource_status::unloaded;

        [[nodiscard]] bool is_unloaded() const noexcept
        {
            return status == resource_status::unloaded;
        }
        [[nodiscard]] bool is_ready() const noexcept
        {
            return status == resource_status::ready;
        }
        [[nodiscard]] bool is_pending() const noexcept
        {
            return status == resource_status::pending;
        }
        [[nodiscard]] bool is_failed() const noexcept
        {
            return status == resource_status::failed;
        }
    };

} // namespace tavros::core

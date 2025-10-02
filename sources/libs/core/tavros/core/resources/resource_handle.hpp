#pragma once

#include <tavros/core/types.hpp>

namespace tavros::core
{

    /**
     * @brief Lightweight handle for a resource in a resource_pool.
     *
     * Encapsulates an ID that contains index and generation to safely refer
     * to objects in a resource_pool.
     */
    template<class T>
    struct resource_handle
    {
        uint32 id = 0xffffffffui32;

        /**
         * @brief Returns an invalid handle.
         */
        static constexpr resource_handle invalid() noexcept
        {
            return {0xffffffffui32};
        }

        constexpr bool operator==(resource_handle other) const noexcept
        {
            return id == other.id;
        }

        constexpr bool operator!=(resource_handle other) const noexcept
        {
            return id != other.id;
        }
    };

} // namespace tavros::core

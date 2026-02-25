#pragma once

namespace tavros::assets
{

    /**
     * @brief Defines access permissions for a resource.
     *
     * Specifies whether a resource is opened for reading, writing,
     * or both. Used when creating or opening resources through a provider.
     */
    enum class asset_open_mode
    {
        /// Asset is opened in read-only mode.
        read_only,

        ///  Resource is opened in write-only mode.
        write_only,
    };

} // namespace tavros::assets

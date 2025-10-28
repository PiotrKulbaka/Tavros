#pragma once

namespace tavros::resources
{

    /**
     * @brief Defines access permissions for a resource.
     *
     * Specifies whether a resource is opened for reading, writing,
     * or both. Used when creating or opening resources through a provider.
     */
    enum class resource_access
    {
        /// Resource is opened in read-only mode.
        read_only,

        ///  Resource is opened in write-only mode.
        write_only,

        ///  Resource is opened in both read and write mode.
        read_write,
    };

} // namespace tavros::resources

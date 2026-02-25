#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/assets/asset_stream.hpp>
#include <tavros/assets/asset_open_mode.hpp>

namespace tavros::assets
{

    /**
     * @brief Abstract interface for accessing and managing streams.
     *
     * An asset provider defines how streams are accessed for reading and writing.
     * Concrete implementations can provide streams from the filesystem, memory archives,
     * or virtual paths.
     */
    class asset_provider
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~asset_provider() noexcept = default;

        /**
         * @brief Checks if the provider allows reading from the specified path.
         *
         * @param path The path to the asset.
         * @return true if the asset can be opened for reading, false otherwise.
         */
        virtual [[nodiscard]] bool can_read(core::string_view path) const noexcept = 0;

        /**
         * @brief Checks if the provider allows writing to the specified path.
         *
         * @param path The path to the asset.
         * @return true if the asset can be opened for writing, false otherwise.
         */
        virtual [[nodiscard]] bool can_write(core::string_view path) const noexcept = 0;

        /**
         * @brief Checks whether an asset exists at the specified path.
         *
         * @param path The path to the asset.
         * @return true if the asset exists, false otherwise.
         *
         * @throws core::file_error If the asset cannot be accessed due to permissions or other I/O errors.
         */
        virtual [[nodiscard]] bool exists(core::string_view path) const = 0;

        /**
         * @brief Opens a stream to the specified asset for reading or writing.
         *
         * @param path The path to the asset.
         * @param access Specifies the desired access mode (read-only, write-only, etc.).
         * @return A unique pointer to the opened stream.
         *
         * @throws core::file_error If opening fails due to I/O errors, permission issues, or invalid access mode.
         */
        virtual [[nodiscard]] core::unique_ptr<asset_stream> open(core::string_view path, asset_open_mode access) = 0;
    };

} // namespace tavros::assets

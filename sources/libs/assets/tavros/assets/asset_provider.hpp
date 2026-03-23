#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/io/stream_reader.hpp>
#include <tavros/core/io/stream_writer.hpp>

namespace tavros::assets
{

    /**
     * @brief Abstract interface for asset providers.
     *
     * An asset provider defines how assets are accessed for reading and writing.
     * Concrete implementations can back assets with a filesystem, memory archive,
     * network source, or any other storage backend.
     *
     * Providers are mounted into an @c asset_manager and searched in mount order
     * to resolve asset paths. Scheme-based routing allows multiple providers to
     * coexist without path conflicts.
     */
    class asset_provider
    {
    public:
        virtual ~asset_provider() noexcept = default;

        /**
         * @brief Returns the URI scheme this provider handles.
         *
         * Used for scheme-based routing. Examples: @c "pack", @c "file", @c "memory".
         * Return an empty @c string_view to participate in the generic fallback chain
         * (no scheme prefix required in the path).
         *
         * @par Examples
         * - @c "pack"  matches @c "pack://textures/grass.png"
         * - @c ""      matches @c "/textures/grass.png"
         */
        [[nodiscard]] virtual core::string_view scheme() const noexcept = 0;

        /**
         * @brief Returns true if the provider can open the asset at @p path for reading.
         *
         * This is a lightweight check and must not throw.
         */
        [[nodiscard]] virtual bool can_read(core::string_view path) const noexcept = 0;

        /**
         * @brief Returns true if the provider can open the asset at @p path for writing.
         *
         * This is a lightweight check and must not throw.
         */
        [[nodiscard]] virtual bool can_write(core::string_view path) const noexcept = 0;

        /**
         * @brief Returns true if an asset exists at @p path.
         *
         * @throws core::file_error If existence cannot be determined due to I/O or permission errors.
         */
        [[nodiscard]] virtual bool exists(core::string_view path) const = 0;

        /**
         * @brief Opens the asset at @p path for reading.
         *
         * @param path The path to the asset.
         * @return A non-null unique pointer to the opened reader.
         *
         * @throws core::file_error If the asset cannot be opened due to I/O or permission errors.
         */
        [[nodiscard]] virtual core::unique_ptr<core::basic_stream_reader> open_reader(core::string_view path) = 0;

        /**
         * @brief Opens the asset at @p path for writing.
         *
         * @param path The path to the asset.
         * @return A non-null unique pointer to the opened writer.
         *
         * @throws core::file_error If the asset cannot be opened due to I/O or permission errors.
         */
        [[nodiscard]] virtual core::unique_ptr<core::basic_stream_writer> open_writer(core::string_view path) = 0;
    };

} // namespace tavros::assets

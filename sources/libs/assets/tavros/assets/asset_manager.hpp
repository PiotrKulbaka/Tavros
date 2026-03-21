#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/containers/fixed_vector.hpp>
#include <tavros/core/ids/handle_allocator.hpp>

#include <tavros/assets/asset_provider.hpp>

namespace tavros::assets
{

    /**
     * @brief Central manager for accessing assets through multiple providers.
     *
     * The asset manager is responsible for locating and opening assets from
     * various sources, such as filesystem, memory archives, or virtual paths.
     * It maintains a list of mounted providers and searches them in order
     * to resolve asset paths.
     *
     * This class is non-copyable and serves purely as a high-level access point
     * to underlying asset streams. It does not decode asset formats.
     */
    class asset_manager : core::noncopyable
    {
    public:
        /**
         * @brief Constructs an empty asset manager.
         */
        asset_manager();

        /**
         * @brief Destroys the asset manager and releases all mounted providers.
         */
        ~asset_manager() noexcept;

        /**
         * @brief Mounts a new asset provider.
         *
         * The provider can be any class derived from `asset_provider`.
         * Mounted providers are searched in the order they were added.
         *
         * @tparam T The type of the provider (must derive from `asset_provider`).
         * @param args Arguments forwarded to the provider's constructor.
         */
        template<typename T, typename... Args>
            requires std::derived_from<T, asset_provider>
        void mount(Args&&... args)
        {
            auto provider = core::make_unique<T>(std::forward<Args>(args)...);
            m_providers.emplace_back(std::move(provider));
        }

        /**
         * @brief Checks whether an asset exists across all mounted providers.
         *
         * @param path The path to the asset.
         * @return true if the asset exists in at least one provider, false otherwise.
         *
         * @throws core::file_error If a provider cannot access the asset due to permissions or other I/O issues.
         */
        bool exists(core::string_view path) const;

        /**
         * @brief Opens a stream to the specified asset.
         *
         * The first provider that allows access to the asset will be used.
         *
         * @param path The path to the asset.
         * @param open_mode The desired access mode (read-only by default).
         * @return A unique pointer to the opened stream.
         *
         * @throws core::file_error If opening fails due to permission issues, invalid open mode, or other I/O errors.
         */
        core::unique_ptr<asset_stream> open(core::string_view path, asset_open_mode open_mode = asset_open_mode::read_only) const;

        /**
         * @brief Attempts to open a stream to the specified asset.
         *
         * The first provider that allows access to the asset will be used.
         * Unlike open(), this function does not throw on failure.
         *
         * @param path The path to the asset.
         * @param open_mode The desired access mode (read-only by default).
         * @return A unique pointer to the opened stream, or nullptr if the asset
         *         could not be opened for any reason.
         *
         * @note This function is noexcept and will return nullptr instead of
         *       throwing if the asset is not found, access is denied,
         *       the open mode is invalid, or any I/O error occurs.
         */
        core::unique_ptr<asset_stream> try_open(core::string_view path, asset_open_mode open_mode = asset_open_mode::read_only) const noexcept;

        /**
         * @brief Reads the entire asset as UTF-8 text.
         *
         * Opens the asset at the specified path and reads its entire contents
         * as a UTF-8 encoded string.
         *
         * @param path Path to the asset.
         * @return A string containing the asset contents.
         *
         * @throws core::file_error If the asset cannot be opened or read.
         */
        core::string read_text(core::string_view path) const;

        /**
         * @brief Reads the entire asset as a binary buffer.
         *
         * Opens the asset at the specified path and reads its entire contents
         * into a contiguous byte buffer.
         *
         * @param path Path to the asset.
         * @return A buffer containing the raw bytes of the asset.
         *
         * @throws core::file_error If the asset cannot be opened or read.
         */
        core::dynamic_buffer<uint8> read_binary(core::string_view path) const;

    private:
        core::vector<core::unique_ptr<asset_provider>> m_providers;
    };

} // namespace tavros::assets

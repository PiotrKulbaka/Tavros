#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/containers/static_vector.hpp>

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
         * @brief Maximum number of mounted providers.
         *
         * Having more providers increases search time for assets. If you need
         * more providers, the search algorithm should be revised.
         */
        constexpr static size_t k_max_providers = 16;

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
         * @brief Reads the entire asset as text.
         *
         * Convenience function that opens the asset and reads all bytes as UTF-8 text.
         *
         * @param path The path to the asset.
         * @return A string containing the asset's contents.
         *
         * @throws core::file_error If opening or reading the asset fails.
         */
        core::string read_text(core::string_view path) const;

        /**
         * @brief Reads the entire asset as a binary buffer.
         *
         * Convenience function that opens the asset and reads all bytes into a vector.
         *
         * @param path The path to the asset.
         * @return A vector containing the raw bytes of the asset.
         *
         * @throws core::file_error If opening or reading the asset fails.
         */
        core::vector<uint8> read_binary(core::string_view path) const;

    private:
        using providers_vector = core::static_vector<core::unique_ptr<asset_provider>, k_max_providers>;
        providers_vector m_providers;
    };

} // namespace tavros::assets

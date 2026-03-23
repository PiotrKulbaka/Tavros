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
     * @brief Central manager for accessing assets through mounted providers.
     *
     * The asset manager locates and opens assets from various sources such as
     * the filesystem, memory archives, or virtual paths. It maintains an ordered
     * list of mounted providers and searches them in mount order to resolve paths.
     *
     * Scheme-based routing is supported: a path prefixed with @c "scheme://" is
     * routed directly to the matching provider. Unprefixed paths fall through to
     * providers with an empty scheme, in mount order.
     *
     * This class is non-copyable and does not decode asset formats - it provides
     * raw byte streams only.
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
         * Constructs a provider of type @p T in-place and appends it to the
         * provider list. Providers are searched in mount order.
         *
         * @tparam T Must derive from @c asset_provider.
         * @param args Arguments forwarded to @c T's constructor.
         */
        template<typename T, typename... Args>
            requires std::derived_from<T, asset_provider>
        void mount(Args&&... args)
        {
            auto provider = core::make_unique<T>(std::forward<Args>(args)...);
            m_providers.emplace_back(std::move(provider));
        }

        /**
         * @brief Returns true if the asset exists in at least one mounted provider.
         *
         * @throws core::file_error If a provider fails to check existence due to I/O or permission errors.
         */
        [[nodiscard]] bool exists(core::string_view path) const;

        /**
         * @brief Opens the asset at @p path for reading.
         *
         * The first provider that reports @c can_read() for the path is used.
         *
         * @return A non-null unique pointer to the opened reader.
         * @throws core::file_error If no provider can open the asset, or on I/O failure.
         */
        core::unique_ptr<core::basic_stream_reader> open_reader(core::string_view path) const;

        /**
         * @brief Opens the asset at @p path for writing.
         *
         * The first provider that reports @c can_write() for the path is used.
         *
         * @return A non-null unique pointer to the opened writer.
         * @throws core::file_error If no provider can open the asset, or on I/O failure.
         */
        core::unique_ptr<core::basic_stream_writer> open_writer(core::string_view path) const;

        /**
         * @brief Attempts to open the asset at @p path for reading.
         *
         * Does not throw. Returns @c nullptr if the asset cannot be opened for any reason.
         */
        core::unique_ptr<core::basic_stream_reader> try_open_reader(core::string_view path) const noexcept;

        /**
         * @brief Attempts to open the asset at @p path for writing.
         *
         * Does not throw. Returns @c nullptr if the asset cannot be opened for any reason.
         */
        core::unique_ptr<core::basic_stream_writer> try_open_writer(core::string_view path) const noexcept;

        /**
         * @brief Opens the asset and reads its entire contents as a UTF-8 string.
         *
         * @throws core::file_error If the asset cannot be opened or read.
         */
        core::string read_text(core::string_view path) const;

        /**
         * @brief Opens the asset and reads its entire contents into a byte buffer.
         *
         * @throws core::file_error If the asset cannot be opened or read.
         */
        core::dynamic_buffer<uint8> read_binary(core::string_view path) const;

    private:
        core::vector<core::unique_ptr<asset_provider>> m_providers;
    };

} // namespace tavros::assets

#pragma once

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/containers/unordered_set.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/tef/registry.hpp>
#include <tavros/tef/source_provider.hpp>

namespace tavros::tef
{

    class loader
    {
    public:
        explicit loader(core::unique_ptr<tef_provider> provider) noexcept
            : m_provider(std::move(provider))
        {
        }

        ~loader() noexcept = default;

        /**
         * @brief Loads a TEFF file and all its transitive includes into a registry.
         *
         * Parse errors are logged via the internal logger.
         *
         * @param path  Absolute path to the root file.
         * @return      Registry containing all loaded documents.
         *              On failure the registry may be partially populated.
         */
        [[nodiscard]] core::unique_ptr<registry> load(core::string_view path);

        /**
         * @brief Loads a TEFF file and all its transitive includes into a registry.
         *
         * @param path    Absolute path to the root file.
         * @param errors  Receives all accumulated error messages.
         * @return        Registry containing all loaded documents.
         *                On failure the registry may be partially populated.
         */
        [[nodiscard]] core::unique_ptr<registry> load(core::string_view path, core::string& errors);

    private:
        core::unique_ptr<tef_provider> m_provider;

        static core::logger s_logger;
    };

} // namespace tavros::tef

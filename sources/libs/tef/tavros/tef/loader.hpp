#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/containers/unordered_set.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/logger/diagnostics.hpp>
#include <tavros/tef/workspace.hpp>
#include <tavros/tef/source_provider.hpp>

namespace tavros::tef
{

    /**
     * @brief TEFF file loader responsible for parsing source files and resolving includes.
     *
     * The loader builds a complete @ref workspace from a root file, including all
     * transitive dependencies introduced via @c @include directives.
     *
     * Parsing errors are either logged internally or collected explicitly depending
     * on the selected overload of @ref load().
     */
    class loader
    {
    public:
        /**
         * @brief Constructs a loader with a custom source provider.
         *
         * @param provider  Ownership of a source provider used to retrieve file contents.
         */
        explicit loader(core::unique_ptr<source_provider> provider) noexcept
            : m_provider(std::move(provider))
        {
        }

        /**
         * @brief Destructor.
         */
        ~loader() noexcept = default;

        /**
         * @brief Loads a TEFF file and all its transitive includes into a workspace.
         *
         * Parsing errors are reported via the internal logger.
         *
         * @param path  Absolute path to the root file.
         *
         * @return A workspace containing all loaded documents.
         *         On failure the workspace may be partially populated.
         */
        [[nodiscard]] core::unique_ptr<workspace> load(core::string_view path);

        /**
         * @brief Loads a TEFF file and all its transitive includes into a workspace.
         *
         * Parsing errors are written into the provided error buffer in addition
         * to being processed internally.
         *
         * @param path    Absolute path to the root file.
         * @param errors  Output string receiving accumulated error messages.
         *
         * @return A workspace containing all loaded documents.
         *         On failure the workspace may be partially populated.
         */
        [[nodiscard]] core::unique_ptr<workspace> load(core::string_view path, core::diagnostics& diagnostics);

    private:
        core::unique_ptr<source_provider> m_provider;
    };

} // namespace tavros::tef

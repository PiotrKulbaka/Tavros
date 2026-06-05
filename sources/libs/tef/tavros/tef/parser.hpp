#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/logger/diagnostics.hpp>
#include <tavros/tef/workspace.hpp>

namespace tavros::tef
{

    /**
     * @brief Result of parsing a TEF document.
     *
     * Contains the status of the parsing operation along with
     * auxiliary data collected during parsing.
     */
    struct parse_result
    {
        /**
         * @brief List of included resources encountered during parsing.
         *
         * Stores paths or identifiers of all @c include directives
         * found in the source. The order is preserved as encountered.
         */
        core::vector<core::string> inclusions;

        /**
         * @brief Deferred inheritance link description.
         *
         * Stores a node together with the prototype path that shall be
         * resolved after the main parse stage.
         */
        struct inherit_proto_t
        {
            /// Target node that declares inheritance.
            node* n = nullptr;

            /// Prototype path referenced by the node.
            core::string path;

            /// Source location for error reporting.
            core::string_view file = {};

            /// Source line for error reporting.
            int32 row = 0;

            /// Source column for error reporting.
            int32 col = 0;
        };

        /**
         * @brief List of nodes requiring inheritance resolution.
         *
         * Each entry describes a node and the prototype path that must
         * be linked after parsing completes.
         */
        core::vector<inherit_proto_t> inheritance;
    };

    /**
     * @brief TEF document parser.
     *
     * Provides functionality to parse a textual TEF source into a node-based
     * document representation.
     *
     * This class is non-instantiable and exposes only static parsing utilities.
     */
    class parser : core::nonconstructable
    {
    public:
        /**
         * @brief Parses a TEF source string into a document node.
         *
         * @param source The input text to parse. Must remain valid for the duration of the call.
         * @param doc Output root node that will be populated with parsed data.
         * @param ds Output string used to store parsing error messages.
         *           The string is appended to, not cleared automatically.
         *           It is the caller's responsibility to clear it if needed.
         *
         * @return parse_result Structure containing the parsing status and collected include directives.
         *
         * @note On failure, @p doc may be partially filled.
         */
        static [[nodiscard]] parse_result parse(core::string_view source, node& doc, core::diagnostics& ds);
    };

} // namespace tavros::tef

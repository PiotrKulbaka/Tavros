#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/string.hpp>
#include <tavros/tef/registry.hpp>

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
         * @brief Indicates whether parsing completed successfully.
         *
         * If @c true, the document was parsed without critical errors.
         * If @c false, the output document may be incomplete or invalid.
         */
        bool success = false;

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
        struct inherit_node_t
        {
            /**
             * @brief Target node that declares inheritance.
             */
            node* n = nullptr;

            /**
             * @brief Prototype path referenced by the node.
             */
            core::string path;
        };

        /**
         * @brief List of nodes requiring inheritance resolution.
         *
         * Each entry describes a node and the prototype path that must
         * be linked after parsing completes.
         */
        core::vector<inherit_node_t> inheritance;
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
         * @param errors Output string used to store parsing error messages.
         *               The string is appended to, not cleared automatically.
         *               It is the caller's responsibility to clear it if needed.
         *
         * @return parse_result Structure containing the parsing status and collected include directives.
         *
         * @note On failure, @p doc may be partially filled.
         */
        static [[nodiscard]] parse_result parse(core::string_view source, node& doc, core::string& errors);
    };

} // namespace tavros::tef

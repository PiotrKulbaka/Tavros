#pragma once

#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/types.hpp>
#include <tavros/tef/registry.hpp>

namespace tavros::tef
{

    /**
     * @brief Serializes TEFF node trees into textual representation.
     *
     * Converts a node hierarchy (or a set of documents from a registry)
     * back into TEFF source format.
     *
     * Serialization is performed in two passes:
     *   1. Estimate the required buffer size to minimize reallocations.
     *   2. Reserve memory and write the output.
     *
     * The serializer does not perform any validation and assumes that
     * the input node tree is well-formed.
     */
    class serializer
    {
    public:
        /**
         * @brief Output formatting mode.
         */
        enum class formatting
        {
            /// Human-readable output with indentation and line breaks.
            pretty,

            /// Compact output with minimal whitespace.
            compact,
        };

        /**
         * @brief Formatting configuration for serialization.
         */
        struct formatting_options
        {
            /// Selected formatting mode.
            formatting fmt = formatting::pretty;

            /// Number of spaces per nesting level (used in pretty mode).
            uint32 nested_indent = 4;

            /// Base indentation applied to the root level (used in pretty mode).
            uint32 base_indent = 0;
        };

    public:
        /**
         * @brief Constructs a serializer with default formatting options.
         */
        serializer() noexcept = default;

        /**
         * @brief Constructs a serializer with custom formatting options.
         */
        explicit serializer(const formatting_options& opt) noexcept
            : m_options(opt)
        {
        }

        ~serializer() noexcept = default;

        /**
         * @brief Returns mutable access to formatting options.
         */
        [[nodiscard]] formatting_options& options() noexcept
        {
            return m_options;
        }

        /**
         * @brief Returns read-only access to formatting options.
         */
        [[nodiscard]] const formatting_options& options() const noexcept
        {
            return m_options;
        }

        /**
         * @brief Serializes all documents stored in a registry.
         *
         * Documents are serialized in their storage order.
         *
         * @param reg Registry containing documents.
         * @return Serialized text.
         */
        [[nodiscard]] core::string serialize_all(const registry& reg);

        /**
         * @brief Serializes a single node (and its subtree) to a string.
         *
         * If @p n is a document node, all its top-level children are serialized.
         * If @p n is an object or scalar, it is serialized as a single element.
         *
         * @param n Node to serialize.
         * @return  Serialized text.
         */
        [[nodiscard]] core::string serialize(const node& n);

        /**
         * @brief Serializes a node and appends the result to @p out.
         *
         * Performs two passes: estimate then fill.
         *
         * @param n    Node to serialize.
         * @param out  Target string. Existing content is preserved.
         */
        void serialize_into(const node& n, core::string& out);

    private:
        /**
         * @brief Estimates the number of characters required to serialize a node
         */
        [[nodiscard]] size_t estimate(const node& n, uint32 depth) const noexcept;

        /**
         * @brief Computes indentation width for a given nesting level.
         */
        [[nodiscard]] size_t indent_for(uint32 nesting_level) const noexcept;

        /**
         * @brief Writes a node (and its subtree) into the output buffer.
         */
        void write_node(const node& n, core::string& out, uint32 depth) const;

        /**
         * @brief Writes a scalar node value into the output buffer.
         */
        void write_scalar(const node& n, core::string& out) const;

    private:
        formatting_options m_options;
    };

} // namespace tavros::tef

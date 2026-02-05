#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    class pp_lexer;

    /**
     * @brief Represents a single token produced by the preprocessor lexer.
     *
     * `pp_token` is a lightweight, non-owning view into the original source
     * code. It stores the token type, its text range, and source location
     * (line and column). No dynamic memory allocations are performed.
     *
     * All returned string views reference the original source buffer, which must
     * remain valid while the token is in use.
     */
    class pp_token
    {
    public:
        /**
         * @brief Enumeration of all supported preprocessor token kinds.
         */
        enum class token_type
        {
            directive_hash,    /// The '#' character starting a preprocessor directive
            directive_name,    /// Begining of preprocessor directive (e.g. #include, #define)
            header_name,       /// Header name only in include directive
            directive_end,     /// End of preprocessor directive
            identifier,        /// Identifier
            number,            /// Numeric literal
            punctuator,        /// Punctuation character
            string_literal,    /// String literal
            character_literal, /// Character literal
            error,             /// Lexing error
            end_of_source,     /// End of source
        };

    public:
        /**
         * @brief Returns the token kind.
         */
        token_type type() const noexcept
        {
            return m_type;
        }

        /**
         * @brief Returns the lexeme text.
         *
         * The returned text depends on the token type:
         * - For string and character literals, the surrounding quotes (`"` or `'`)
         *   are not included.
         * - For header names, the surrounding delimiters (`"` or `< >`) are not included.
         *
         * The returned view always refers to the original source buffer.
         */
        core::string_view lexeme() const noexcept
        {
            return m_lexeme;
        }

        /**
         * @brief Returns the full source line containing the token.
         */
        core::string_view line() const noexcept
        {
            return m_line;
        }

        /**
         * @brief Returns the 1-based line number of the token.
         */
        int32 row() const noexcept
        {
            return m_row;
        }

        /**
         * @brief Returns the 1-based column number of the token.
         */
        int32 col() const noexcept
        {
            return m_col;
        }

        /**
         * @brief Returns an error description if the token represents an error.
         */
        core::string_view error_string() const noexcept
        {
            return m_error;
        }

    private:
        pp_token(
            token_type        type,
            core::string_view lexeme,
            core::string_view line,
            int32             row,
            int32             col,
            core::string_view error = {}
        ) noexcept
            : m_type(type)
            , m_lexeme(lexeme)
            , m_line(line)
            , m_row(row)
            , m_col(col)
            , m_error(error)
        {
        }

    private:
        token_type        m_type;
        core::string_view m_lexeme;
        core::string_view m_line;
        int32             m_row;
        int32             m_col;
        core::string_view m_error;

        friend class pp_lexer;
    };

} // namespace tavros::renderer

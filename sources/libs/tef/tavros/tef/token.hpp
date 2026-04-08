#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::tef
{

    class lexer;

    /**
     * @brief Represents a single token produced by the lexer.
     *
     * `token` is a lightweight, non-owning view into the original source
     * code. It stores the token type, its text range, and source location
     * (line and column). No dynamic memory allocations are performed.
     *
     * All returned string views reference the original source buffer, which must
     * remain valid while the token is in use.
     */
    class token
    {
    private:
        friend class lexer;

    public:
        /**
         * @brief Enumeration of all supported tokens.
         */
        enum class token_type
        {
            /// The '@' character starting a directive at the beginning of a line
            directive_at,

            /// The 'end' directive at the end of line
            directive_end,

            /// Identifier or directive name
            identifier,

            /// Numeric literal
            number,

            /// Punctuation character
            punctuator,

            /// String literal
            string_literal,

            /// Lexing error
            error,

            /// End of source
            end_of_source,
        };

    public:
        token() noexcept = default;
        ~token() noexcept = default;

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
         * - For string and character literals, the surrounding quotes `"` are not included.
         * - For header names, the surrounding delimiters `"` are not included.
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
        token(
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
        token_type        m_type = token_type::error;
        core::string_view m_lexeme;
        core::string_view m_line;
        int32             m_row = -1;
        int32             m_col = -1;
        core::string_view m_error;
    };

} // namespace tavros::tef

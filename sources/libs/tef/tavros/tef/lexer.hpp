#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/tef/token.hpp>

namespace tavros::tef
{

    /**
     * @brief Lexer for the TEFF (Tavros Engine File Format) language.
     *
     * Scans a range of UTF-8 source text and produces a sequence of tokens.
     * The lexer does not validate token values - for example, a number token
     * may contain an arbitrary sequence of digits, letters, dots and underscores.
     * Validation and conversion are the responsibility of the parser.
     *
     * The lexer does not own the source memory and performs no dynamic
     * allocations. The source range must remain valid for the lifetime of
     * the lexer.
     *
     * Token types produced:
     *   - identifier      - key or keyword (true, false, null)
     *   - number          - numeric literal sequence (validated by parser)
     *   - string_literal  - quoted string, content without quotes
     *   - punctuator      - '=', ':', '{', '}', '.'
     *   - directive_at    - '@' at start of line, begins a directive
     *   - directive_end   - implicit token emitted after a directive line ends
     *   - end_of_source   - emitted once when the source is exhausted
     *   - error           - lexical error with a description
     */
    class lexer : core::noncopyable
    {
    public:
        /**
         * @brief Constructs a lexer for a given source range.
         *
         * @param begin Pointer to the first character of the source.
         * @param end   Pointer one past the last character of the source.
         */
        lexer(const char* begin, const char* end) noexcept;

        /**
         * @brief Default destructor. Nothing to release since no dynamic memory is used.
         */
        ~lexer() noexcept = default;

        /**
         * @brief Returns the next token from the source.
         *
         * After @ref token::token_type::end_of_source is returned, subsequent
         * calls continue to return end_of_source tokens.
         *
         * @return The next token.
         */
        const token& next_token() noexcept
        {
            m_current_token = scan_next_token();
            return m_current_token;
        }

        /**
         * @brief Returns the last scanned token (current token).
         */
        [[nodiscard]] const token& current_token() const noexcept
        {
            return m_current_token;
        }

        /**
         * @brief Returns true if the lexer has emitted end_of_source.
         */
        bool end_of_source() const noexcept
        {
            return m_end_of_source;
        }

    private:
        token scan_next_token() noexcept;

        void               adv(size_t n = 1) noexcept;
        void               adv_ln() noexcept;
        [[nodiscard]] char peek(size_t n = 0) const noexcept;
        [[nodiscard]] bool eos(size_t n = 0) const noexcept;
        [[nodiscard]] bool more(size_t n = 0) const noexcept;
        [[nodiscard]] bool match(core::string_view sv) const noexcept;

        token scan_special() noexcept;
        token scan_punctuation() noexcept;
        token scan_identifier() noexcept;
        token scan_number() noexcept;
        token scan_string_literal() noexcept;
        token scan_at() noexcept;
        void  skip_trivia() noexcept;

        static token make_error(core::string_view line, int32 row, int32 col, core::string_view error) noexcept
        {
            return {token::token_type::error, {}, line, row, col, false, error};
        }

    private:
        const char* m_begin;
        const char* m_end;
        const char* m_forward;

        core::string_view m_line;

        int32 m_row;
        int32 m_col;

        bool m_is_line_start;
        bool m_end_of_source;

        token m_current_token;
    };

} // namespace tavros::tef

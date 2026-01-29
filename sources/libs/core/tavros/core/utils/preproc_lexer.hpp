#pragma once

#include <tavros/core/utils/preproc_token.hpp>

namespace tavros::core
{

    /**
     * @brief A lexer for the GLSL preprocessor.
     *
     * `preproc_lexer` scans a range of source code and produces individual
     * preprocessor tokens, including directives, identifiers, numbers,
     * string/character literals, punctuation, and header names. It handles
     * whitespace, comments, and tracks line/column positions for each token.
     *
     * @note The lexer does not own the memory it scans and performs no dynamic
     *       memory allocations. It is the caller's responsibility to ensure
     *       that the source memory remains valid for the lifetime of the lexer.
     */
    class preproc_lexer
    {
    public:
        /**
         * @brief Constructs a lexer for a given source range.
         * @param begin Pointer to the first character of the source code.
         * @param end   Pointer one past the last character of the source code.
         */
        preproc_lexer(const char* begin, const char* end) noexcept;

        /**
         * @brief Default destructor. Nothing to release since no dynamic memory is used.
         */
        ~preproc_lexer() = default;

        /**
         * @brief Returns the next token from the source code.
         * @return The next `preproc_token`, or `end_of_file` if at the end.
         */
        [[nodiscard]] preproc_token next_token() noexcept;

        /**
         * @brief Checks if the lexer has reached the end of the source.
         * @return `true` if fully lexed, `false` otherwise.
         */
        bool end_of_source() const noexcept
        {
            return eos();
        }

    private:
        void adv() noexcept;
        void adv_ln() noexcept;
        char peek() const noexcept;
        char peek(size_t n) const noexcept;
        bool eos() const noexcept;
        bool eos(size_t n) const noexcept;

        // Also scan character literal
        preproc_token scan_identifier() noexcept;
        preproc_token scan_number() noexcept;
        preproc_token scan_string_literal() noexcept;
        preproc_token scan_directive() noexcept;
        preproc_token scan_header_name() noexcept;

        void skip_trivia() noexcept;
        void update_end_of_line() noexcept;

        void skip_to_new_line();

    private:
        const char* m_begin;
        const char* m_end;
        const char* m_forward;

        const char* m_line_begin;
        const char* m_line_end;
        char        m_last_nonspace_char;

        size_t m_row;
        size_t m_col;

        bool m_in_dir; // Inside the directive
        bool m_required_end_dir;

        bool m_header_name_expected;
        bool m_line_break;
        bool m_is_start_of_line;
        bool m_unclosed_multiline_comment;
    };

} // namespace tavros::core

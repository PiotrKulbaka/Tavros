#pragma once

#include <tavros/renderer/shaders/pp_token.hpp>

namespace tavros::renderer
{

    /**
     * @brief A lexer for the GLSL preprocessor.
     *
     * `pp_lexer` scans a range of source code and produces individual
     * preprocessor tokens, including directives, identifiers, numbers,
     * string/character literals, punctuation, and header names. It handles
     * whitespace, comments, and tracks line/column positions for each token.
     *
     * @note The lexer does not own the memory it scans and performs no dynamic
     *       memory allocations. It is the caller's responsibility to ensure
     *       that the source memory remains valid for the lifetime of the lexer.
     */
    class pp_lexer
    {
    public:
        /**
         * @brief Constructs a lexer for a given source range.
         * @param begin Pointer to the first character of the source code.
         * @param end   Pointer one past the last character of the source code.
         */
        pp_lexer(const char* begin, const char* end) noexcept;

        /**
         * @brief Default destructor. Nothing to release since no dynamic memory is used.
         */
        ~pp_lexer() = default;

        /**
         * @brief Returns the next token from the source code.
         * @return The next `pp_token`, or `end_of_file` if at the end.
         */
        [[nodiscard]] pp_token next_token() noexcept;

        /**
         * @brief Checks if the lexer has reached the end of the source.
         * @return `true` if fully lexed, `false` otherwise.
         */
        bool end_of_source() const noexcept
        {
            return m_end_of_source;
        }

    private:
        void adv() noexcept;
        void adv_ln() noexcept;
        char peek() const noexcept;
        char peek(size_t n) const noexcept;
        bool eos() const noexcept;
        bool eos(size_t n) const noexcept;

        // Also scan character literal
        pp_token scan_punctuation() noexcept;
        pp_token scan_identifier() noexcept;
        pp_token scan_number() noexcept;
        pp_token scan_string_literal() noexcept;
        pp_token scan_hash() noexcept;
        pp_token scan_directive() noexcept;
        pp_token scan_header_name() noexcept;

        void skip_trivia() noexcept;
        void update_end_of_line() noexcept;

        void skip_to_new_line();

        static pp_token make_error(core::string_view line, int32 row, int32 col, core::string_view error) noexcept
        {
            return {pp_token::token_type::error, {}, line, row, col, error};
        }

    private:
        const char* m_begin;
        const char* m_end;
        const char* m_forward;

        core::string_view m_line;
        const char*       m_last_nonspace_char;

        int32 m_row;
        int32 m_col;

        bool m_in_hash;          // Inside a '#' preprocessor hash
        bool m_in_dir;           // Inside the directive
        bool m_required_end_dir; // Required directive end
        bool m_end_of_source;

        bool m_header_name_expected;
        bool m_line_break;
        bool m_is_start_of_line;
        bool m_unclosed_multiline_comment;
    };

} // namespace tavros::renderer

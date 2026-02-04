#include <tavros/core/utils/pp_lexer.hpp>

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <cctype>

namespace
{
#define CHK(x) (static_cast<int>(x) >= -1 && static_cast<int>(x) <= 255)

    template<class CharT>
    unsigned char char_cast(CharT c) noexcept
    {
        return static_cast<unsigned char>(c);
    }

    template<class CharT>
    bool is_whitespace(CharT c) noexcept
    {
        return std::isspace(char_cast(c)) != 0;
    }

    template<class CharT>
    bool is_start_identifier(CharT c) noexcept
    {
        return std::isalpha(char_cast(c)) != 0 || c == '_';
    }

    template<class CharT>
    bool is_part_identifier(CharT c) noexcept
    {
        return is_start_identifier(c) || std::isdigit(char_cast(c)) != 0;
    }

    template<class CharT>
    bool is_start_number(CharT c) noexcept
    {
        return std::isdigit(char_cast(c)) != 0;
    }

    template<class CharT>
    bool is_part_number(CharT c) noexcept
    {
        return is_part_identifier(c);
    }

    template<class CharT>
    bool is_punct(CharT c) noexcept
    {
        return std::ispunct(char_cast(c)) != 0;
    }

    using tt = tavros::core::pp_token::token_type;

} // namespace

namespace tavros::core
{

    pp_lexer::pp_lexer(const char* begin, const char* end) noexcept
        : m_begin(begin)
        , m_end(end)
        , m_forward(begin)
        , m_line(begin, begin)
        , m_row(1)
        , m_col(1)
        , m_in_hash(false)
        , m_in_dir(false)
        , m_required_end_dir(false)
        , m_end_of_source(false)
        , m_header_name_expected(false)
        , m_line_break(false)
        , m_is_start_of_line(true)
        , m_unclosed_multiline_comment(false)
    {
        update_end_of_line();
    }

    pp_token pp_lexer::next_token() noexcept
    {
        // Placeholder implementation: returns end_of_source token
        if (m_header_name_expected) {
            return scan_header_name();
        }

        do {
            auto line = m_line;
            auto row = m_row;
            auto col = m_col;
            const auto* fwd = m_forward;

            skip_trivia();

            if (m_required_end_dir) {
                m_required_end_dir = false;
                return {tt::directive_end, {}, line, row, col};
            }

            if (eos()) {
                break;
            }

            // Identifier or directive name
            if (is_start_identifier(peek())) {
                if (m_in_hash) {
                    return scan_directive();
                }
                return scan_identifier();
            }

            // Number
            if (is_start_number(peek())) {
                return scan_number();
            }

            // String literal or character literal
            if (peek() == '\'' || peek() == '"') {
                return scan_string_literal();
            }

            // Preprocessor directive or punctuation
            if (peek() == '#') {
                return scan_hash();
            }

            // Line splicing
            if ((m_in_dir || m_in_hash) && m_forward == m_last_nonspace_char && peek() == '\\') {
                adv();
                continue;
            }

            return scan_punctuation();
        } while (true);

        if (m_unclosed_multiline_comment) {
            m_unclosed_multiline_comment = false;
            return make_error(m_line, m_row, m_col, "Unclosed multi-line comment at end of source");
        }

        if (m_in_dir) {
            m_in_dir = false;
            return {tt::directive_end, {m_forward, m_forward}, m_line, m_row, m_col};
        }

        m_end_of_source = true;
        return {tt::end_of_source, {}, m_line, m_row, m_col};
    }

    void pp_lexer::adv() noexcept
    {
        TAV_ASSERT(!eos());
        TAV_ASSERT(peek() != '\n');

        ++m_forward;
        ++m_col;
    }

    void pp_lexer::adv_ln() noexcept
    {
        TAV_ASSERT(!eos());
        TAV_ASSERT(peek() == '\n');

        m_col = 1;
        ++m_row;
        ++m_forward;
        m_header_name_expected = false;
        m_is_start_of_line = true;
        m_line = {m_forward, m_forward};

        if (m_in_dir && *m_last_nonspace_char != '\\') {
            m_in_dir = false;
            m_required_end_dir = true;
        }

        update_end_of_line();
    }

    char pp_lexer::peek() const noexcept
    {
        TAV_ASSERT(!eos());
        return *m_forward;
    }

    char pp_lexer::peek(size_t n) const noexcept
    {
        TAV_ASSERT(m_forward + n < m_end);
        return *(m_forward + n);
    }

    bool pp_lexer::eos() const noexcept
    {
        return m_forward >= m_end;
    }

    bool pp_lexer::eos(size_t n) const noexcept
    {
        return m_forward + n >= m_end;
    }

    pp_token pp_lexer::scan_identifier() noexcept
    {
        TAV_ASSERT(is_start_identifier(peek()));
        TAV_ASSERT(!m_in_hash);

        m_is_start_of_line = false;
        auto        col = m_col;
        const auto* beg = m_forward;

        do {
            adv();
        } while (!eos() && is_part_identifier(peek()));

        return {tt::identifier, {beg, m_forward}, m_line, m_row, col};
    }

    pp_token pp_lexer::scan_punctuation() noexcept
    {
        TAV_ASSERT(!eos());

        const auto* beg = m_forward;
        auto        col = m_col;
        m_is_start_of_line = false;
        m_in_hash = false;
        adv();


        if (is_punct(*beg)) {
            return {tt::punctuator, {beg, m_forward}, m_line, m_row, col};
        }
        return {tt::error, {beg, m_forward}, m_line, m_row, col};
    }

    pp_token pp_lexer::scan_number() noexcept
    {
        TAV_ASSERT(is_start_number(peek()));

        m_is_start_of_line = false;
        auto        col = m_col;
        const auto* beg = m_forward;

        do {
            adv();
        } while (!eos() && is_part_number(peek()));

        return {tt::number, {beg, m_forward}, m_line, m_row, col};
    }

    pp_token pp_lexer::scan_string_literal() noexcept
    {
        TAV_ASSERT(peek() == '\'' || peek() == '"');

        m_is_start_of_line = false;
        auto ln = m_row;
        auto col = m_col;

        // String literal or character literal
        const auto quote_char = peek();
        adv();
        const auto* beg = m_forward;
        const auto* end = m_end;

        while (!eos() && peek() != '\n' && peek() != quote_char) {
            if (peek() == '\\') {
                // Escape sequence
                adv();
                if (!eos() && peek() != '\n') {
                    adv();
                } else {
                    // Error: incomplete escape sequence
                    return make_error(m_line, m_row, m_col, "Incomplete escape sequence");
                }
            } else {
                adv();
            }
        }

        if (eos()) {
            return make_error(m_line, m_row, m_col, "Unexpected end of file");
        }

        if (peek() == quote_char) {
            // Closing quote found
            const auto* end_of_str = m_forward;
            adv();

            return {quote_char == '"' ? tt::string_literal : tt::character_literal, {beg, end_of_str}, m_line, ln, col};
        }

        // Error: unterminated string/character literal
        TAV_ASSERT(peek() != quote_char);
        string_view err_msg = quote_char == '"' ? "Unterminated string literal" : "Unterminated character literal";
        return make_error(m_line, m_row, m_col, err_msg);
    }

    pp_token pp_lexer::scan_hash() noexcept
    {
        TAV_ASSERT(peek() == '#');

        auto        is_start_line = m_is_start_of_line;
        const auto* hash_beg = m_forward;
        auto        hash_col = m_col;
        m_is_start_of_line = false;
        adv();

        if (is_start_line) {
            // Directive hash can be only at the start of the line
            m_in_hash = true;
            return {tt::directive_hash, {hash_beg, m_forward}, m_line, m_row, hash_col};
        }

        // Just a punctuation token
        return {tt::punctuator, {hash_beg, m_forward}, m_line, m_row, hash_col};
    }

    pp_token pp_lexer::scan_directive() noexcept
    {
        TAV_ASSERT(m_in_hash);
        TAV_ASSERT(!m_in_dir);
        TAV_ASSERT(!eos());
        TAV_ASSERT(is_start_identifier(peek()));

        constexpr string_view dir_include_name = "include";
        const auto*           dir_beg = m_forward;
        auto                  dir_col = m_col;
        auto                  dir_row = m_row;

        do {
            adv();
        } while (!eos() && is_part_identifier(peek()));

        // Check if it's the "include" directive
        string_view sv{dir_beg, m_forward};
        if (sv == dir_include_name) {
            m_header_name_expected = true;
        }

        m_in_dir = true;
        m_in_hash = false;

        return {tt::directive_name, {dir_beg, m_forward}, m_line, dir_row, dir_col};
    }

    pp_token pp_lexer::scan_header_name() noexcept
    {
        m_header_name_expected = false;
        m_is_start_of_line = false;

        auto before_row = m_row;
        auto before_col = m_col;
        auto before_line = m_line;
        skip_trivia();

        // Check if we are still on the same line
        if (m_row != before_row || eos()) {
            return make_error(before_line, before_row, before_col, "Unexpected end of line in header name");
        }

        // Header name must start with < or "
        if (peek() != '<' && peek() != '"') {
            auto err = make_error(m_line, m_row, m_col, "Expected header name after #include directive");
            skip_to_new_line();
            return err;
        }

        // Scan header name
        const char closing_char = peek() == '<' ? '>' : '"';
        const auto col = m_col;
        const auto row = m_row;
        auto line = m_line;
        adv();
        const auto* beg = m_forward;

        while (!eos() && peek() != '\n' && peek() != closing_char) {
            adv();
        }

        if (eos()) {
            return make_error(m_line, m_row, m_col, "Unexpected end of source in header name");
        }

        if (peek() == '\n') {
            return make_error(m_line, m_row, m_col, "Unexpected end of line in header name");
        }

        const auto* end = m_forward;

        // Closing quote found
        adv();

        return {tt::header_name, {beg, end}, line, row, col};
    }

    void pp_lexer::skip_trivia() noexcept
    {
        while (!eos()) {
            if (peek() == '\n') {
                adv_ln();
            } else if (is_whitespace(peek())) {
                adv();
            } else if (peek() == '/' && !eos(1)) {
                if (peek(1) == '*') {
                    // Multi-line comment started
                    adv();
                    adv();

                    // necessary to check for an unclosed comment at EOF
                    m_unclosed_multiline_comment = true;

                    // Skip until end of comment
                    while (!eos()) {
                        if (peek() == '*' && !eos(1) && peek(1) == '/') {
                            // End of multi-line comment found
                            adv();
                            adv();
                            m_unclosed_multiline_comment = false;
                            break;
                        }
                        if (peek() == '\n') {
                            adv_ln();
                        } else {
                            adv();
                        }
                    }
                } else if (peek(1) == '/') {
                    // Single-line comment
                    adv();
                    adv();

                    // Skip to end of line
                    while (!eos()) {
                        if (peek() == '\n') {
                            auto ch = *m_last_nonspace_char;
                            adv_ln();
                            if (ch != '\\') {
                                if (m_in_dir) {
                                    m_required_end_dir = true;
                                }
                                break;
                            }
                        } else {
                            adv();
                        }
                    }
                } else {
                    // Not trivia
                    return;
                }
            } else {
                // Not trivia
                return;
            }
        }
    }

    void pp_lexer::update_end_of_line() noexcept
    {
        const auto* p = m_line.data();
        m_last_nonspace_char = p;

        while (p < m_end && *p != '\n') {
            if (!is_whitespace(*p)) {
                m_last_nonspace_char = p;
            }
            ++p;
        }

        m_line = {m_line.data(), p};
    }

    void pp_lexer::skip_to_new_line()
    {
        while (!eos() && peek() != '\n') {
            adv();
        }

        if (!eos() && peek() == '\n') {
            adv_ln();
        }
    }

} // namespace tavros::core

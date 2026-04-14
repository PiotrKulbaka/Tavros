#include <tavros/tef/lexer.hpp>

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <cctype>

namespace
{

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
        return std::isalnum(char_cast(c)) != 0 || c == '.' || c == '_';
    }

    template<class CharT>
    bool is_punct(CharT c) noexcept
    {
        return std::ispunct(char_cast(c)) != 0;
    }

    using tt = tavros::tef::token::token_type;
} // namespace

namespace tavros::tef
{

    lexer::lexer(const char* begin, const char* end) noexcept
        : m_begin(begin)
        , m_end(end)
        , m_forward(begin)
        , m_line(begin, begin)
        , m_row(1)
        , m_col(1)
        , m_is_line_start(true)
        , m_end_of_source(false)
    {
        const auto* end_line = begin;
        while (end_line < m_end && *end_line != '\n') {
            ++end_line;
        }

        m_line = {begin, end_line};
    }

    token lexer::scan_next_token() noexcept
    {
        skip_trivia();

        if (eos()) {
            m_end_of_source = true;
            return {tt::end_of_source, {}, m_line, m_row, m_col, false};
        }

        // Check for special tokens (inf, +inf, -inf, nan, true, false)
        auto special_tok = scan_special();
        if (special_tok.type() != tt::none) {
            return special_tok;
        }

        // Identifier or directive name
        if (is_start_identifier(peek())) {
            return scan_identifier();
        }

        // Number
        if (is_start_number(peek()) || peek() == '-' || peek() == '+') {
            return scan_number();
        }

        // String literal or character literal
        if (peek() == '"') {
            return scan_string_literal();
        }

        // Directive or punctuation
        if (peek() == '@') {
            return scan_at();
        }

        return scan_punctuation();
    }

    void lexer::adv(size_t n) noexcept
    {
        TAV_ASSERT(n >= 1);
        TAV_ASSERT(more(n - 1));
#if TAV_DEBUG
        for (size_t i = 0; i < n; ++i) {
            TAV_ASSERT(peek(i) != '\n');
        }
#endif

        m_forward += n;
        m_col += static_cast<int32>(n);
    }

    void lexer::adv_ln() noexcept
    {
        TAV_ASSERT(more());
        TAV_ASSERT(peek() == '\n');

        m_col = 1;
        ++m_row;
        ++m_forward;
        m_is_line_start = true;

        const auto* end_line = m_forward;
        while (end_line < m_end && *end_line != '\n') {
            ++end_line;
        }
        m_line = {m_forward, end_line};
    }

    char lexer::peek(size_t n) const noexcept
    {
        TAV_ASSERT(more(n));
        return *(m_forward + n);
    }

    bool lexer::eos(size_t n) const noexcept
    {
        return (m_forward + n) >= m_end;
    }

    bool lexer::more(size_t n) const noexcept
    {
        return (m_forward + n) < m_end;
    }

    bool lexer::match(core::string_view sv) const noexcept
    {
        if (core::string_view{m_forward, m_end}.starts_with(sv)) {
            if (more(sv.size())) {
                // Ensure that the matched keyword is not a prefix of a longer identifier
                return !is_part_identifier(peek(sv.size()));
            }
            return true;
        }
        return false;
    }

    token lexer::scan_identifier() noexcept
    {
        TAV_ASSERT(is_start_identifier(peek()));

        auto is_line_start = m_is_line_start;
        m_is_line_start = false;

        auto        col = m_col;
        const auto* beg = m_forward;

        do {
            adv();
        } while (more() && is_part_identifier(peek()));
        return {tt::identifier, {beg, m_forward}, m_line, m_row, col, is_line_start};
    }

    token lexer::scan_special() noexcept
    {
        auto is_line_start = m_is_line_start;
        m_is_line_start = false;

        auto        col = m_col;
        const auto* beg = m_forward;

        if (match("inf")) {
            adv(3);
            return {tt::number, {beg, m_forward}, m_line, m_row, col, is_line_start};
        }

        if (match("+inf")) {
            adv(4);
            return {tt::number, {beg, m_forward}, m_line, m_row, col, is_line_start};
        }

        if (match("-inf")) {
            adv(4);
            return {tt::number, {beg, m_forward}, m_line, m_row, col, is_line_start};
        }

        if (match("nan")) {
            adv(3);
            return {tt::number, {beg, m_forward}, m_line, m_row, col, is_line_start};
        }

        if (match("true")) {
            adv(4);
            return {tt::keyword, {beg, m_forward}, m_line, m_row, col, is_line_start};
        }

        if (match("false")) {
            adv(5);
            return {tt::keyword, {beg, m_forward}, m_line, m_row, col, is_line_start};
        }

        return {};
    }

    token lexer::scan_punctuation() noexcept
    {
        TAV_ASSERT(more());

        auto is_line_start = m_is_line_start;
        m_is_line_start = false;

        const auto* beg = m_forward;
        auto        col = m_col;
        adv();

        if (is_punct(*beg)) {
            return {tt::punctuation, {beg, m_forward}, m_line, m_row, col, is_line_start};
        }

        return {tt::error, {beg, m_forward}, m_line, m_row, col, is_line_start, "Unexpected character"};
    }

    token lexer::scan_number() noexcept
    {
        TAV_ASSERT(is_start_number(peek()) || peek() == '-' || peek() == '+');

        auto is_line_start = m_is_line_start;
        m_is_line_start = false;

        auto        col = m_col;
        const auto* beg = m_forward;

        // Optional sign
        if (peek() == '-' || peek() == '+') {
            adv();
        }

        auto punct_tok = token{tt::punctuation, {beg, m_forward}, m_line, m_row, col, is_line_start};

        if (eos()) {
            // Just a pinctuation
            return punct_tok;
        }

        do {
            adv();
        } while (more() && is_part_number(peek()));

        if (more() && peek() == '.') {
            // Skip dot and scan fractional part
            do {
                adv();
            } while (more() && is_part_number(peek()));
        }

        return {tt::number, {beg, m_forward}, m_line, m_row, col, is_line_start};
    }

    token lexer::scan_string_literal() noexcept
    {
        TAV_ASSERT(peek() == '"');

        auto is_line_start = m_is_line_start;
        m_is_line_start = false;

        auto ln = m_row;
        auto col = m_col;

        adv(); // skip opening '"'
        const auto* beg = m_forward;

        while (more() && peek() != '\n' && peek() != '"') {
            if (peek() == '\\') {
                // Escape sequence
                adv(); // skip backslash
                if (eos() || peek() == '\n') {
                    return make_error(m_line, m_row, m_col, "Incomplete escape sequence");
                }
                adv(); // skip any escaped character
            } else {
                adv();
            }
        }

        if (eos()) {
            return make_error(m_line, m_row, m_col, "Unexpected end of source in string literal");
        }

        if (peek() == '\n') {
            return make_error(m_line, m_row, m_col, "Unterminated string literal");
        }

        TAV_ASSERT(peek() == '"');
        const auto* end_of_str = m_forward;
        adv(); // skip closing '"'

        return {tt::string_literal, {beg, end_of_str}, m_line, ln, col, is_line_start};
    }

    token lexer::scan_at() noexcept
    {
        TAV_ASSERT(peek() == '@');

        auto is_line_start = m_is_line_start;
        m_is_line_start = false;

        const auto* at_beg = m_forward;
        auto        col = m_col;

        adv(); // skip '@'

        if (more() && is_start_identifier(peek())) {
            const auto* dir_beg = m_forward;
            // Directive name
            do {
                adv();
            } while (more() && is_part_identifier(peek()));

            return {tt::directive, {dir_beg, m_forward}, m_line, m_row, col, is_line_start};
        }

        // Just a punctuation token
        return {tt::punctuation, {at_beg, m_forward}, m_line, m_row, col, is_line_start};
    }

    void lexer::skip_trivia() noexcept
    {
        while (more()) {
            if (peek() == '\n') {
                adv_ln();
            } else if (is_whitespace(peek())) {
                adv();
            } else if (peek() == '#') {
                // Line comment - skip to end of line
                do {
                    adv();
                } while (more() && peek() != '\n');
            } else {
                // Not trivia
                return;
            }
        }
    }

} // namespace tavros::tef

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
        , m_is_start_of_line(true)
        , m_in_dir(false)
        , m_required_end_dir(false)
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
        do {
            auto line = m_line;
            auto row = m_row;
            auto col = m_col;

            skip_trivia();

            // Emit pending directive_end produced by skip_trivia
            if (m_required_end_dir) {
                m_required_end_dir = false;
                return {tt::directive_end, {}, line, row, col};
            }

            if (eos()) {
                break;
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
        } while (true);

        if (m_in_dir) {
            m_in_dir = false;
            return {tt::directive_end, {m_forward, m_forward}, m_line, m_row, m_col};
        }

        m_end_of_source = true;
        return {tt::end_of_source, {}, m_line, m_row, m_col};
    }

    void lexer::adv() noexcept
    {
        TAV_ASSERT(!eos());
        TAV_ASSERT(peek() != '\n');

        ++m_forward;
        ++m_col;
    }

    void lexer::adv_ln() noexcept
    {
        TAV_ASSERT(!eos());
        TAV_ASSERT(peek() == '\n');

        m_col = 1;
        ++m_row;
        ++m_forward;
        m_is_start_of_line = true;

        if (m_in_dir) {
            m_in_dir = false;
            m_required_end_dir = true;
            return;
        }

        const auto* end_line = m_forward;
        while (end_line < m_end && *end_line != '\n') {
            ++end_line;
        }
        m_line = {m_forward, end_line};
    }

    char lexer::peek() const noexcept
    {
        TAV_ASSERT(!eos());
        return *m_forward;
    }

    bool lexer::eos() const noexcept
    {
        return m_forward >= m_end;
    }

    token lexer::scan_identifier() noexcept
    {
        TAV_ASSERT(is_start_identifier(peek()));

        m_is_start_of_line = false;
        auto        col = m_col;
        const auto* beg = m_forward;

        do {
            adv();
        } while (!eos() && is_part_identifier(peek()));

        return {tt::identifier, {beg, m_forward}, m_line, m_row, col};
    }

    token lexer::scan_punctuation() noexcept
    {
        TAV_ASSERT(!eos());

        const auto* beg = m_forward;
        auto        col = m_col;
        m_is_start_of_line = false;
        adv();

        if (is_punct(*beg)) {
            return {tt::punctuator, {beg, m_forward}, m_line, m_row, col};
        }
        return {tt::error, {beg, m_forward}, m_line, m_row, col, "Unexpected character"};
    }

    token lexer::scan_number() noexcept
    {
        TAV_ASSERT(is_start_number(peek()) || peek() == '-' || peek() == '+');

        m_is_start_of_line = false;
        auto        col = m_col;
        const auto* beg = m_forward;

        // Optional sign
        if (peek() == '-' || peek() == '+') {
            adv();
        }

        if (eos() || !is_part_number(peek())) {
            // Just a pinctuation
            return {tt::punctuator, {beg, m_forward}, m_line, m_row, col};
        }

        do {
            adv();
        } while (!eos() && is_part_number(peek()));

        if (!eos() && peek() == '.') {
            // Skip dot and scan fractional part
            do {
                adv();
            } while (!eos() && is_part_number(peek()));
        }

        return {tt::number, {beg, m_forward}, m_line, m_row, col};
    }

    token lexer::scan_string_literal() noexcept
    {
        TAV_ASSERT(peek() == '"');

        m_is_start_of_line = false;
        auto ln = m_row;
        auto col = m_col;

        adv(); // skip opening '"'
        const auto* beg = m_forward;

        while (!eos() && peek() != '\n' && peek() != '"') {
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

        return {tt::string_literal, {beg, end_of_str}, m_line, ln, col};
    }

    token lexer::scan_at() noexcept
    {
        TAV_ASSERT(peek() == '@');

        auto        is_start_line = m_is_start_of_line;
        const auto* at_beg = m_forward;
        auto        col = m_col;

        m_is_start_of_line = false;
        adv(); // skip '@'

        if (is_start_line) {
            // Directive '@ can be only at the start of the line
            m_in_dir = true;
            return {tt::directive_at, {at_beg, m_forward}, m_line, m_row, col};
        }

        // Just a punctuation token
        return {tt::punctuator, {at_beg, m_forward}, m_line, m_row, col};
    }

    void lexer::skip_trivia() noexcept
    {
        while (!eos()) {
            if (peek() == '\n') {
                adv_ln();
            } else if (is_whitespace(peek())) {
                adv();
            } else if (peek() == '#') {
                // Line comment - skip to end of line
                do {
                    adv();
                } while (!eos() && peek() != '\n');
            } else {
                // Not trivia
                return;
            }
        }
    }

} // namespace tavros::tef

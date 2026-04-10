#include <tavros/tef/parser.hpp>

#include <tavros/tef/lexer.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/fixed_string.hpp>
#include <charconv>
#include <limits>

namespace
{
    using tt = tavros::tef::token::token_type;

    bool is_punct(const tavros::tef::token& t, char c) noexcept
    {
        return t.type() == tt::punctuator && t.lexeme().size() == 1 && t.lexeme()[0] == c;
    }

    bool is_bool_literal(tavros::core::string_view sv) noexcept
    {
        return sv == "true" || sv == "false";
    }

    bool is_valid_key(const tavros::tef::token& t) noexcept
    {
        return t.type() == tt::identifier && !is_bool_literal(t.lexeme());
    }

    bool is_value_start(const tavros::tef::token& t) noexcept
    {
        return t.type() == tt::number || t.type() == tt::string_literal || t.type() == tt::keyword || is_punct(t, '{');
    }

    struct parser_impl
    {
        tavros::tef::lexer&   lex;
        tavros::core::string& errors;
        tavros::tef::node&    root;

        bool has_error = false;

        const tavros::tef::token& tok() const noexcept
        {
            return lex.current_token();
        }
        
        bool at_end() const noexcept
        {
            return tok().type() == tt::end_of_source;
        }

        void advance() noexcept
        {
            lex.next_token();
        }

        bool check(tt t) const noexcept
        {
            return tok().type() == t;
        }

        bool check_p(char c) const noexcept
        {
            return is_punct(tok(), c);
        }

        bool consume_if(char c) noexcept
        {
            if (check_p(c)) {
                advance();
                return true;
            }
            return false;
        }

        void report_error(const tavros::tef::token& t, tavros::core::string_view msg)
        {
            errors.append(root.value_or<tavros::core::string_view>({}));
            errors.append(tavros::core::fixed_string<64>::format("({}) : ", t.row()));
            errors.append(msg);
            errors.append("\n");
            has_error = true;
        }

        void skip_directive()
        {
            while (!at_end() && !check(tt::directive_end)) {
                advance();
            }
            if (check(tt::directive_end)) {
                advance();
            }
        }

        void parse_includes(tavros::core::vector<tavros::core::string>& includes)
        {
            while (check(tt::directive_at)) {
                auto at_tok = tok();
                advance();

                if (check(tt::identifier) && tok().lexeme() == "include") {
                    advance();
                    if (check(tt::string_literal)) {
                        includes.emplace_back(tok().lexeme());
                        advance();
                    } else {
                        report_error(tok(), "[E-09] Expected include path");
                    }
                } else {
                    report_error(at_tok, "[E-09] Unknown directive or missing name");
                }

                skip_directive();
            }
        }

        void parse_keyword(tavros::tef::node& parent, tavros::core::string_view key)
        {
            auto sv = tok().lexeme();
            if (sv == "true") {
                parent.append(key, true);
            } else if (sv == "false") {
                parent.append(key, false);
            } else if (sv == "inf" || sv == "+inf") {
                parent.append(key, std::numeric_limits<double>::infinity());
            } else if (sv == "-inf") {
                parent.append(key, -std::numeric_limits<double>::infinity());
            } else if (sv == "nan") {
                parent.append(key, std::numeric_limits<double>::quiet_NaN());
            } else {
                report_error(tok(), "Unknown keyword literal");
            }
            advance();
        }

        void parse_number(tavros::tef::node& parent, tavros::core::string_view key)
        {
            const auto& t = tok();
            const auto  lexeme = t.lexeme();
            const char* beg = lexeme.data();
            const char* end = beg + lexeme.size();

            if (lexeme.size() > 2 && lexeme[0] == '0' && (lexeme[1] == 'x' || lexeme[1] == 'X')) {
                int64 v = 0;
                auto [ptr, ec] = std::from_chars(beg + 2, end, v, 16);
                if (ec == std::errc{} && ptr == end) {
                    parent.append(key, v);
                } else {
                    report_error(t, "[E-06] Invalid hex number");
                }
            } else if (lexeme.size() > 2 && lexeme[0] == '0' && (lexeme[1] == 'b' || lexeme[1] == 'B')) {
                int64 v = 0;
                auto [ptr, ec] = std::from_chars(beg + 2, end, v, 2);
                if (ec == std::errc{} && ptr == end) {
                    parent.append(key, v);
                } else {
                    report_error(t, "[E-06] Invalid binary number");
                }
            } else {
                int64 v_int = 0;
                auto [ptr_i, ec_i] = std::from_chars(beg, end, v_int);
                if (ec_i == std::errc{} && ptr_i == end) {
                    parent.append(key, v_int);
                } else {
                    double v_flt = 0;
                    auto [ptr_f, ec_f] = std::from_chars(beg, end, v_flt);
                    if (ec_f == std::errc{} && ptr_f == end) {
                        parent.append(key, v_flt);
                    } else {
                        report_error(t, "[E-06] Invalid numeric literal");
                    }
                }
            }
            advance();
        }

        void parse_object(tavros::tef::node& parent, tavros::core::string_view key)
        {
            auto open_tok = tok();
            advance();

            auto* obj = parent.append_object(key);
            parse_elements(*obj, false);

            if (consume_if('}')) {
                return;
            }

            report_error(open_tok, "[E-05] Unclosed '{' - expected '}'");
        }

        void parse_single_value(tavros::tef::node& parent, tavros::core::string_view key)
        {
            if (check_p('{')) {
                parse_object(parent, key);
            } else if (check(tt::number)) {
                parse_number(parent, key);
            } else if (check(tt::keyword)) {
                parse_keyword(parent, key);
            } else if (check(tt::string_literal)) {
                parent.append(key, tok().lexeme());
                advance();
            } else {
                report_error(tok(), "Expected valid value");
                advance();
            }
        }

        void parse_optional_inheritance()
        {
            if (!consume_if(':')) {
                return;
            }

            if (!check(tt::identifier)) {
                report_error(tok(), "[E-12] Expected identifier after ':'");
                return;
            }
            advance();

            while (consume_if('.')) {
                if (!check(tt::identifier)) {
                    report_error(tok(), "[E-12] Expected identifier after '.' in path");
                    break;
                }
                advance();
            }
        }

        void parse_keyed_sequence(tavros::tef::node& parent)
        {
            auto key_tok = tok();
            auto key = key_tok.lexeme();
            advance();

            parse_optional_inheritance();

            if (!consume_if('=')) {
                report_error(key_tok, "[E-02] Missing '=' after key");
                if (is_valid_key(tok())) {
                    return;
                }
                if (!at_end() && !check_p('}')) {
                    advance();
                }
                return;
            }

            if (at_end() || check_p('}') || is_valid_key(tok()) || !is_value_start(tok())) {
                report_error(key_tok, "[E-04] Missing value after '='");
                if (!is_valid_key(tok()) && !check_p('}') && !at_end()) {
                    advance();
                }
                return;
            }

            parse_single_value(parent, key);

            while (!at_end() && !check_p('}') && !is_valid_key(tok())) {
                if (!is_value_start(tok())) {
                    break;
                }
                parse_single_value(parent, "");
            }
        }

        void parse_element(tavros::tef::node& parent)
        {
            if (check(tt::directive_at)) {
                report_error(tok(), "[E-09] Directives must appear before elements");
                skip_directive();
            } else if (check(tt::error)) {
                report_error(tok(), tok().error_string());
                advance();
            } else if (is_valid_key(tok())) {
                parse_keyed_sequence(parent);
            } else if (is_value_start(tok())) {
                parse_single_value(parent, "");
            } else {
                report_error(tok(), "[E-05] Unexpected token");
                advance();
            }
        }

        void parse_elements(tavros::tef::node& parent, bool is_root)
        {
            while (!at_end()) {
                if (check_p('}')) {
                    if (is_root) {
                        report_error(tok(), "[E-05] Unexpected '}' at root level");
                        advance();
                        continue;
                    } else {
                        return;
                    }
                }
                parse_element(parent);
            }
        }
    };

} // namespace

namespace tavros::tef
{
    parse_result parser::parse(core::string_view source, node& doc, core::string& errors)
    {
        TAV_ASSERT(doc.parent() == nullptr);

        lexer lex(source.data(), source.data() + source.size());

        parse_result result;
        parser_impl  p{lex, errors, doc};

        p.advance();
        p.parse_includes(result.includes);

        p.parse_elements(doc, true);

        result.success = !p.has_error;
        return result;
    }
} // namespace tavros::tef

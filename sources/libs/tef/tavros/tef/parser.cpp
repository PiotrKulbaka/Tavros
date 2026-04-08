#include <tavros/tef/parser.hpp>

#include <tavros/core/debug/assert.hpp>

#include <charconv>
#include <limits>

namespace
{
    using tt = tavros::tef::token::token_type;

    void append_error(tavros::core::string& err_buf, const tavros::tef::token& tok, tavros::core::string_view message)
    {
        err_buf += "(";
        err_buf += std::to_string(tok.row());
        err_buf += ") : ";
        err_buf += message;
        err_buf += "\n";
    }

    bool check_punct(const tavros::tef::token& tok, char c) noexcept
    {
        return tok.type() == tt::punctuator && tok.lexeme().size() == 1 && tok.lexeme()[0] == c;
    }

    struct parser_impl
    {
        tavros::tef::lexer&   lex;
        tavros::core::string& errors;

        bool has_error = false;
        bool seen_element = false;

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
            return ::check_punct(tok(), c);
        }

        void set_error(const tavros::tef::token& t, tavros::core::string_view msg)
        {
            append_error(errors, t, msg);
            has_error = true;
        }

        bool parse_includes(tavros::core::vector<tavros::core::string>& includes)
        {
            while (check(tt::directive_at)) {
                advance(); // '@'

                if (!check(tt::identifier)) {
                    set_error(tok(), "Expected directive name after '@'");
                    return false;
                }

                auto name = tok().lexeme();
                advance();

                if (name != "include") {
                    set_error(tok(), "Unknown directive");
                    return false;
                }

                if (!check(tt::string_literal)) {
                    set_error(tok(), "Expected include path");
                    return false;
                }

                includes.emplace_back(tok().lexeme());
                advance();

                if (!check(tt::directive_end)) {
                    set_error(tok(), "Expected end of directive");
                    return false;
                }

                advance();
            }

            return true;
        }

        bool parse_number(tavros::tef::node& parent, tavros::core::string_view key)
        {
            const auto& t = tok();
            const auto  lexeme = t.lexeme();

            const char* beg = lexeme.data();
            const char* end = beg + lexeme.size();

            // hex
            if (lexeme.size() > 2 && lexeme[0] == '0' && (lexeme[1] == 'x' || lexeme[1] == 'X')) {
                int64 v = 0;
                auto [ptr, ec] = std::from_chars(beg + 2, end, v, 16);
                if (ec == std::errc{} && ptr == end) {
                    parent.append(key, v);
                    advance();
                    return true;
                }
                set_error(t, "Invalid hex number");
                return false;
            }

            // int
            {
                int64 v = 0;
                auto [ptr, ec] = std::from_chars(beg, end, v);
                if (ec == std::errc{} && ptr == end) {
                    parent.append(key, v);
                    advance();
                    return true;
                }
            }

            // float
            {
                double v = 0;
                auto [ptr, ec] = std::from_chars(beg, end, v);
                if (ec == std::errc{} && ptr == end) {
                    parent.append(key, v);
                    advance();
                    return true;
                }
            }

            set_error(t, "Invalid numeric literal");
            return false;
        }

        bool parse_value(tavros::tef::node& parent, tavros::core::string_view key)
        {
            if (check_p('{')) {
                return parse_object(parent, key) != nullptr;
            }

            if (check(tt::number)) {
                return parse_number(parent, key);
            }

            if (check(tt::string_literal)) {
                parent.append(key, tok().lexeme());
                advance();
                return true;
            }

            if (check(tt::identifier)) {
                auto sv = tok().lexeme();

                if (sv == "true") {
                    parent.append(key, true);
                    advance();
                    return true;
                }
                if (sv == "false") {
                    parent.append(key, false);
                    advance();
                    return true;
                }

                set_error(tok(), "Identifier must be followed by '='");
                return false;
            }

            if (check(tt::error)) {
                set_error(tok(), tok().error_string());
                return false;
            }

            set_error(tok(), "Expected value");
            return false;
        }

        tavros::tef::node* parse_object(tavros::tef::node& parent, tavros::core::string_view key)
        {
            auto open = tok();
            advance(); // {

            auto* obj = parent.append_object(key);

            if (!parse_elements(*obj)) {
                return nullptr;
            }

            if (!check_p('}')) {
                set_error(open, "Expected '}'");
                return nullptr;
            }

            advance();
            return obj;
        }

        bool parse_element(tavros::tef::node& parent)
        {
            if (check(tt::identifier)) {
                auto key = tok().lexeme();
                auto key_tok = tok();

                // bool literals as values
                if (key == "true" || key == "false") {
                    return parse_value(parent, {});
                }

                advance();

                if (!check_p('=')) {
                    set_error(key_tok, "Expected '=' after identifier");
                    return false;
                }

                advance();

                if (!parse_value(parent, key)) {
                    return false;
                }

                while (!at_end() && !check_p('}')) {
                    if (check(tt::identifier)) {
                        auto sv = tok().lexeme();
                        if (sv != "true" && sv != "false") {
                            break;
                        }
                    }

                    if (!parse_value(parent, {})) {
                        return false;
                    }
                }

                return true;
            }

            return parse_value(parent, {});
        }

        bool parse_elements(tavros::tef::node& parent)
        {
            while (!at_end() && !check_p('}')) {
                if (check(tt::directive_at)) {
                    set_error(tok(), "Directives must appear before elements");
                    return false;
                }

                if (!parse_element(parent)) {
                    return false;
                }
            }

            return true;
        }
    };

} // namespace

namespace tavros::tef
{

    parse_result parser::parse(core::string_view source, node& doc)
    {
        lexer lex(source.data(), source.data() + source.size());

        parse_result result;
        parser_impl  p{lex, result.error_str};

        p.advance();
        p.parse_includes(result.includes);
        p.parse_elements(doc);

        if (p.has_error) {
            result.success = false;
        } else {
            result.success = true;
        }

        return result;
    }

} // namespace tavros::tef

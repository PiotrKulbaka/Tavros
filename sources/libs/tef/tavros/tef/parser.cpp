#include <tavros/tef/parser.hpp>

#include <tavros/tef/token.hpp>
#include <tavros/tef/lexer.hpp>
#include <tavros/tef/registry.hpp>

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/fixed_string.hpp>
#include <tavros/core/containers/fixed_vector.hpp>

#include <charconv>
#include <limits>

namespace
{
    using tt = tavros::tef::token::token_type;

    enum class parsing_state
    {
        any,
        in_directive,
        in_key,
        in_colon,
        in_eq,
        in_proto_path,
        in_value,
    };

    using ps = parsing_state;

    using inheritance_t = tavros::core::vector<tavros::tef::parse_result::inherit_node_t>;
    using includes_t = tavros::core::vector<tavros::core::string>;

    struct parser_impl
    {
        using lexer = tavros::tef::lexer;
        using token = tavros::tef::token;
        using node = tavros::tef::node;
        using registry = tavros::tef::registry;
        using string_view = tavros::core::string_view;
        using string = tavros::core::string;
        using small_string = tavros::core::fixed_string<256>;

        lexer&         lex;
        string&        errors;
        node&          root;
        includes_t&    inclusions;
        inheritance_t& inheritance;

        ps   state = ps::any;
        bool has_error = false;
        bool in_file_header = true;

        string_view                     pending_key = {};
        tavros::core::fixed_string<512> pending_proto = {};

        tavros::core::fixed_vector<node*, tavros::tef::registry::k_max_nesting_level> stack;

        // gcc-style error reporting
        void report_error(string_view code, string_view msg)
        {
            const auto& t = lex.current_token();
            auto        file = root.value_or<string_view>({});
            auto        line = t.line();

            if (!file.empty()) {
                errors.append(file);
                errors.append(":");
            }
            errors.append(tavros::core::fixed_string<512>::format("{}:{}: error: {}: {}\n", t.row(), t.col(), code, msg));

            // Source line
            if (!line.empty()) {
                errors.append("    ");
                errors.append(line);
                errors.append("\n");

                // Caret pointing to column
                errors.append("    ");
                auto col = static_cast<size_t>(t.col() > 0 ? t.col() - 1 : 0);
                errors.append(col, ' ');
                errors.append("^\n");
            }

            has_error = true;
        }

        void adv() noexcept
        {
            lex.next_token();
        }

        bool check(tt t) const noexcept
        {
            return lex.current_token().type() == t;
        }

        string_view lexeme() const noexcept
        {
            return lex.current_token().lexeme();
        }

        bool check_p(char c) const noexcept
        {
            return check(tt::punctuation) && !lexeme().empty() && lexeme()[0] == c;
        }

        bool is_directive(string_view name) const noexcept
        {
            return check(tt::directive) && lexeme() == name;
        }

        bool is_value() const noexcept
        {
            return check(tt::keyword) || check(tt::number) || check(tt::string_literal) || check_p('{');
        }

        void to_state(ps new_state) noexcept
        {
            TAV_ASSERT(new_state != state);
            state = new_state;
        }

        void append_number(node& parent)
        {
            auto        key = pending_key;
            auto        lx = lexeme();
            const char* beg = lx.data();
            const char* end = beg + lx.size();

            // Hex: 0x...
            if (lx.size() > 2 && beg[0] == '0' && (beg[1] == 'x' || beg[1] == 'X')) {
                int64 v = 0;
                auto [ptr, ec] = std::from_chars(beg + 2, end, v, 16);
                if (ec == std::errc{} && ptr == end) {
                    parent.append(key, v);
                    return;
                }
                report_error("E-06", small_string::format("invalid hexadecimal literal '{}': value out of range [0, 2^64-1]", lx));
                return;
            }

            // Binary: 0b...
            if (lx.size() > 2 && beg[0] == '0' && (beg[1] == 'b' || beg[1] == 'B')) {
                int64 v = 0;
                auto [ptr, ec] = std::from_chars(beg + 2, end, v, 2);
                if (ec == std::errc{} && ptr == end) {
                    parent.append(key, v);
                    return;
                }
                report_error("E-06", small_string::format("invalid binary literal '{}': value out of range [0, 2^64-1]", lx));
                return;
            }

            // Special float values
            if (lx == "inf" || lx == "+inf") {
                parent.append(key, std::numeric_limits<double>::infinity());
                return;
            }
            if (lx == "-inf") {
                parent.append(key, -std::numeric_limits<double>::infinity());
                return;
            }
            if (lx == "nan") {
                parent.append(key, std::numeric_limits<double>::quiet_NaN());
                return;
            }

            // Integer
            {
                int64 v = 0;
                auto [ptr, ec] = std::from_chars(beg, end, v);
                if (ec == std::errc{} && ptr == end) {
                    parent.append(key, v);
                    return;
                }
                if (ec == std::errc::result_out_of_range) {
                    report_error("E-06", small_string::format("integer literal '{}' is out of range [-(2^63), 2^63-1]", lx));
                    return;
                }
            }

            // Float
            {
                double v = 0.0;
                auto [ptr, ec] = std::from_chars(beg, end, v);
                if (ec == std::errc{} && ptr == end) {
                    parent.append(key, v);
                    return;
                }
            }

            report_error("E-06", small_string::format("invalid numeric literal '{}'", lx));
        }

        void append_value()
        {
            node& parent = *stack.back();

            if (!pending_key.empty()) {
                const auto* n = parent.child(pending_key);
                if (n != nullptr) {
                    report_error("E-01", small_string::format("duplicate key '{}' within the same node", pending_key));
                }
            }

            if (check_p('{')) {
                if (stack.size() >= stack.max_size()) {
                    // Nesting limit exceeded - still create the node to allow
                    // continued parsing, but report the error
                    report_error("E-18", small_string::format("maximum nesting depth ({}) exceeded", tavros::tef::registry::k_max_nesting_level));

                    // Create node in current parent without pushing
                    node* n = parent.append_object(pending_key);
                    inheritance.push_back({n, string(pending_proto)});
                } else {
                    node* n = parent.append_object(pending_key);
                    inheritance.push_back({n, string(pending_proto)});
                    stack.push_back(n);
                }
            } else {
                if (!pending_proto.empty()) {
                    report_error("E-10", small_string::format("prototype reference '{}' specified for a scalar value: "
                                                              "only object nodes may have a prototype reference",
                                                              pending_proto));
                }

                if (check(tt::keyword)) {
                    string_view kw = lexeme();
                    if (kw == "true") {
                        parent.append(pending_key, true);
                    } else if (kw == "false") {
                        parent.append(pending_key, false);
                    } else {
                        report_error("E-16", small_string::format("reserved keyword '{}' cannot be used as a value here", kw));
                    }
                } else if (check(tt::number)) {
                    append_number(parent);
                } else if (check(tt::string_literal)) {
                    parent.append(pending_key, lexeme());
                } else {
                    TAV_UNREACHABLE();
                }
            }

            pending_key = {};
            pending_proto.clear();
        }

        void parse()
        {
            stack.push_back(&root);
            adv();

            while (!check(tt::end_of_source)) {
                TAV_ASSERT(!check(tt::none));

                if (check(tt::error)) {
                    // Lex errors always report and advance
                    auto e = lex.current_token().error_string();

                    // Map lexer errors to error codes
                    if (e.find("escape") != string_view::npos) {
                        report_error("E-08", small_string::format("unrecognized escape sequence in string literal: {}", e));
                    } else if (e.find("Unterminated") != string_view::npos || e.find("Newline") != string_view::npos) {
                        report_error("E-07", small_string::format("newline inside string literal (string must be closed on the same line): {}", e));
                    } else {
                        report_error("E-03", small_string::format("lexical error: {}", e));
                    }
                    adv();
                    continue;
                }

                // Validate punctuation characters
                if (check(tt::punctuation)) {
                    auto p = lexeme();
                    if (p.empty() || !(p[0] == '=' || p[0] == ':' || p[0] == '{' || p[0] == '}' || p[0] == '.')) {
                        report_error("E-03", small_string::format("unexpected character '{}': not a valid punctuator", p));
                        adv();
                        continue;
                    }
                }

                switch (state) {
                case ps::any:
                    if (check(tt::identifier)) {
                        in_file_header = false;
                        to_state(ps::in_key);
                        break;
                    }
                    if (is_value()) {
                        in_file_header = false;
                        to_state(ps::in_value);
                        break;
                    }
                    if (check(tt::directive)) {
                        to_state(ps::in_directive);
                        break;
                    }
                    if (check_p('}')) {
                        if (stack.size() > 1) {
                            stack.pop_back();
                        } else {
                            report_error("E-05", "unexpected '}': no matching '{' found at the top level");
                        }
                        adv();
                        break;
                    }

                    report_error("E-03", small_string::format("unexpected token '{}': expected a key, value, or directive", lexeme()));
                    adv();
                    break;

                case ps::in_directive:
                    TAV_ASSERT(check(tt::directive));

                    if (is_directive("include")) {
                        adv();
                        if (check(tt::string_literal)) {
                            if (!in_file_header) {
                                report_error("E-09",
                                             "'@include' directive must appear before any element: "
                                             "move all @include directives to the top of the file");
                            } else {
                                inclusions.push_back(string(lexeme()));
                            }
                            adv();
                        } else {
                            report_error("E-14", small_string::format("'@include' requires a string literal file path, got '{}'", lexeme()));
                        }
                    } else {
                        report_error("E-17", small_string::format("unknown directive '@{}': "
                                                                  "supported directives are: @include, @abstract",
                                                                  lexeme()));
                        adv();
                    }

                    to_state(ps::any);
                    break;

                case ps::in_key:
                    TAV_ASSERT(check(tt::identifier));

                    pending_key = lexeme();
                    adv();

                    if (check_p(':')) {
                        to_state(ps::in_colon);
                    } else if (check_p('=')) {
                        to_state(ps::in_eq);
                    } else {
                        report_error("E-02", small_string::format("expected '=' or ':' after key '{}', got '{}'", pending_key, lexeme()));
                        pending_key = {};
                        to_state(ps::any);
                    }
                    break;

                case ps::in_colon:
                    TAV_ASSERT(check_p(':'));
                    adv();

                    if (check(tt::identifier)) {
                        to_state(ps::in_proto_path);
                    } else {
                        report_error("E-12", small_string::format("expected prototype path after ':', got '{}': "
                                                                  "prototype path must be a dot-separated sequence of identifiers "
                                                                  "(e.g. 'base_widget' or 'ui.base_widget')",
                                                                  lexeme()));
                        pending_key = {};
                        to_state(ps::any);
                    }
                    break;

                case ps::in_eq:
                    TAV_ASSERT(check_p('='));
                    adv();

                    if (is_value()) {
                        to_state(ps::in_value);
                    } else {
                        report_error("E-04", small_string::format("expected a value after '=' for key '{}', got '{}': "
                                                                  "valid values are: integer, float, boolean (true/false), "
                                                                  "string literal, or object '{{...}}'",
                                                                  string_view(pending_key), lexeme()));
                        pending_key = {};
                        to_state(ps::any);
                    }
                    break;

                case ps::in_proto_path:
                    TAV_ASSERT(check(tt::identifier));

                    pending_proto.append(lexeme());
                    adv();

                    // Consume dot-separated path segments
                    while (check_p('.')) {
                        adv();
                        if (!check(tt::identifier)) {
                            report_error("E-12", small_string::format("expected identifier after '.' in prototype path '{}', got '{}': "
                                                                      "prototype path segments must be valid identifiers",
                                                                      pending_proto, lexeme()));
                            break;
                        }
                        pending_proto.append(".");
                        pending_proto.append(lexeme());
                        adv();
                    }

                    if (check_p('=')) {
                        to_state(ps::in_eq);
                    } else {
                        report_error("E-02", small_string::format("expected '=' after prototype path '{}' for key '{}', got '{}': "
                                                                  "syntax is: key : prototype_path = {{ ... }}",
                                                                  pending_proto, pending_key, lexeme()));
                        pending_key = {};
                        pending_proto.clear();
                        to_state(ps::any);
                    }
                    break;

                case ps::in_value:
                    TAV_ASSERT(is_value());

                    in_file_header = false;
                    append_value();
                    adv();
                    to_state(ps::any);
                    break;

                default:
                    TAV_UNREACHABLE();
                }
            }

            // End of source - check for unclosed nodes
            if (stack.size() > 1) {
                report_error("E-05", small_string::format("unexpected end of source: {} unclosed node(s) - "
                                                          "check for missing '}}' brace(s)",
                                                          stack.size() - 1));
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
        parser_impl  p{lex, errors, doc, result.inclusions, result.inheritance};
        p.parse();

        result.success = !p.has_error;
        return result;
    }
} // namespace tavros::tef

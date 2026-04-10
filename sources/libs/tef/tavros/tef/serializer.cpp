#include <tavros/tef/serializer.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <tavros/core/fixed_string.hpp>

#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>

namespace tavros::tef
{

    core::string serializer::serialize_all(const registry& reg)
    {
        core::string result;
        for (auto& doc : reg.documents()) {
            result.append("# ");
            result.append(doc.value_or<core::string_view>({}));
            result.append("\n");
            serialize_into(doc, result);
        }
        return result;
    }

    core::string serializer::serialize(const node& n)
    {
        core::string out;
        serialize_into(n, out);
        return out;
    }

    void serializer::serialize_into(const node& n, core::string& out)
    {
        const size_t estimate_size = estimate(n, 0);
        out.reserve(out.size() + estimate_size);
        write_node(n, out, 0);
    }

    size_t serializer::estimate(const node& n, uint32 nesting_level) const noexcept
    {
        size_t sz = 0;

        const bool is_continuation = !n.has_key();

        if (n.is_document()) {
            for (const auto& child : n.children()) {
                sz += estimate(child, nesting_level);
            }
            return sz;
        }

        // Indent only for keyed nodes
        if (!is_continuation) {
            sz += indent_for(nesting_level);
        }

        sz += n.has_key() ? n.key().size() + 3 : 0;

        if (n.is_container()) {
            sz += 4; // braces + separators
            for (const auto& child : n.children()) {
                sz += estimate(child, nesting_level + 1);
            }
            sz += indent_for(nesting_level);
        } else {
            if (n.is_integer()) {
                sz += sizeof("-9223372036854775808");
            } else if (n.is_floating_point()) {
                sz += sizeof("-1.7976931348623157e+308");
            } else if (n.is_boolean()) {
                sz += sizeof("false");
            } else if (n.is_string()) {
                const auto sv = n.value_or<core::string_view>({});
                sz += sv.size() * 2 + 2;
            }
        }

        sz += 1; // space or newline after value
        return sz;
    }

    size_t serializer::indent_for(uint32 nesting_level) const noexcept
    {
        if (m_options.fmt == formatting::compact) {
            return 0;
        }
        return static_cast<size_t>(m_options.base_indent) + static_cast<size_t>(nesting_level) * m_options.nested_indent;
    }

    void serializer::write_node(const node& n, core::string& out, uint32 nesting_level) const
    {
        const bool pretty = formatting::pretty == m_options.fmt;

        if (n.is_document()) {
            auto filename = n.value_or<core::string_view>({});
            for (const auto& child : n.children()) {
                write_node(child, out, nesting_level);
            }
            return;
        }

        // Keyless node — continuation of a value sequence.
        // Indent is suppressed; caller already placed a space before us.
        const bool is_continuation = !n.has_key();
        const bool is_first_child = n.prev() == nullptr;

        if (!is_continuation || is_first_child) {
            out.append(indent_for(nesting_level), ' ');
        }

        // Write key
        if (n.has_key()) {
            out.append(n.key());
            out.append(pretty ? " = " : "=");
        }

        if (n.is_container()) {
            if (n.has_children()) {
                out.append(pretty ? "{\n" : "{");
                for (const auto& child : n.children()) {
                    write_node(child, out, nesting_level + 1);
                }
                out.append(indent_for(nesting_level), ' ');
                out.append("}");
            } else {
                out.append("{}");
            }
            // After closing brace — check if next sibling is a continuation
            const node* nx = n.next();
            if (pretty) {
                if (nx && !nx->has_key()) {
                    out.append(" ");
                } else {
                    out.append("\n");
                }
            } else {
                if (nx && !nx->has_key()) {
                    out.append(" ");
                }
            }
        } else {
            write_scalar(n, out);
            // After scalar — check if next sibling is a continuation
            const node* nx = n.next();
            if (pretty) {
                if (nx && !nx->has_key()) {
                    out.append(" ");
                } else {
                    out.append("\n");
                }
            } else {
                if (nx && !nx->has_key()) {
                    out.append(" ");
                }
            }
        }
    }

    void serializer::write_scalar(const node& n, core::string& out) const
    {
        if (n.is_integer()) {
            const auto val = n.value_or<int64>(0);

            auto s = core::fixed_string<64>::format("{}", val);
            out.append(s);

        } else if (n.is_floating_point()) {
            const auto val = n.value_or<double>(0.0);

            if (std::isnan(val)) {
                out.append("nan");
            } else if (std::isinf(val)) {
                out.append((val > 0.0) ? "inf" : "-inf");
            } else {
                auto s = core::fixed_string<64>::format("{}", val);
                if (s.find('.') == s.npos && s.find('e') == s.npos && s.find('E') == s.npos) {
                    s.append(".0");
                }
                out.append(s);
            }

        } else if (n.is_boolean()) {
            out.append(n.value_or<bool>(false) ? "true" : "false");

        } else if (n.is_string()) {
            const auto sv = n.value_or<core::string_view>({});

            out.append("\"");
            for (const char c : sv) {
                switch (c) {
                case '\'':
                    out.append("\\\'");
                    break;
                case '\"':
                    out.append("\\\"");
                    break;
                case '\\':
                    out.append("\\\\");
                    break;
                case '\n':
                    out.append("\\n");
                    break;
                case '\t':
                    out.append("\\t");
                    break;
                case '\r':
                    out.append("\\r");
                    break;
                default:
                    out += c;
                    break;
                }
            }
            out.append("\"");

        } else {
            TAV_UNREACHABLE();
        }
    }

} // namespace tavros::tef

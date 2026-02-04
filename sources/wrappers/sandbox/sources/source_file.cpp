#include "source_file.hpp"

#include <tavros/core/utils/pp_lexer.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/assert.hpp>

namespace
{
    static tavros::core::logger logger("source_file");

    using tt = tavros::core::pp_token::token_type;
} // namespace

namespace tavros::renderer
{

    source_file::source_file(core::string source, core::string_view path)
        : m_source(std::move(source))
    {
        auto succes = scan(path);
        if (!succes) {
            throw std::runtime_error("source_file: failed to scan '" + core::string(path) + "'");
        }
    }

    size_t source_file::text_parts_count() const noexcept
    {
        return m_includes.size() + 1;
    }

    core::string_view source_file::text_part(size_t index) const noexcept
    {
        TAV_ASSERT(index < text_parts_count());

        if (m_includes.size() == 0) {
            return m_source;
        }

        if (index == 0) { // Head
            return {m_source.c_str(), m_includes.front().replace_begin};
        }

        if (index == m_includes.size()) { // Tail
            auto off = m_includes.back().replace_begin + m_includes.back().replace_size;
            return {m_source.c_str() + off, m_source.size() - off};
        }

        auto off = m_includes[index - 1].replace_begin + m_includes[index - 1].replace_size;
        auto size = m_includes[index].replace_begin - off;

        return {m_source.c_str() + off, size};
    }

    int32 source_file::line_number_for_text_part(size_t index) const noexcept
    {
        TAV_ASSERT(index < text_parts_count());

        if (index == 0) {
            return 1;
        }

        return m_includes[index - 1].line_number;
    }

    size_t source_file::includes_count() const noexcept
    {
        return m_includes.size();
    }

    core::string_view source_file::include_path(size_t index) const noexcept
    {
        TAV_ASSERT(index < includes_count());

        return {m_source.c_str() + m_includes[index].path_begin, m_includes[index].path_size};
    }

    core::string_view source_file::source() const noexcept
    {
        return m_source;
    }

    bool source_file::scan(core::string_view path)
    {
        tavros::core::pp_lexer lex(m_source.c_str(), m_source.c_str() + m_source.size());

        enum class include_scan_state
        {
            idle,
            seen_hash,
            seen_include,
            seen_header,
        };

        auto state = include_scan_state::idle;

        const auto*  source_begin = m_source.c_str();
        include_info current = {};
        bool         success = true;

        do {
            auto tok = lex.next_token();

            if (tok.type() == tt::error) {
                ::logger.error("Error: {}:{}:{}: {}", path, tok.row(), tok.col(), tok.error_string());
                ::logger.error("In line: {}", tok.line());
                success = false;
            }

            switch (state) {
            case include_scan_state::idle:
                if (tok.type() == tt::directive_hash) {
                    state = include_scan_state::seen_hash;
                    current.replace_begin = static_cast<size_t>(tok.lexeme().data() - source_begin);
                }
                break;

            case include_scan_state::seen_hash:
                if (tok.type() == tt::directive_name && tok.lexeme() == tavros::core::string_view("include")) {
                    state = include_scan_state::seen_include;
                } else {
                    state = include_scan_state::idle;
                }
                break;

            case include_scan_state::seen_include:
                if (tok.type() == tt::header_name) {
                    state = include_scan_state::seen_header;
                    current.path_begin = static_cast<size_t>(tok.lexeme().data() - source_begin);
                    current.path_size = static_cast<size_t>(tok.lexeme().size());
                    current.replace_size = static_cast<size_t>(tok.lexeme().data() - source_begin) - current.replace_begin + tok.lexeme().size() + 1;
                    current.line_number = tok.row();
                } else {
                    state = include_scan_state::idle;
                }
                break;

            case include_scan_state::seen_header:
                if (tok.type() == tt::directive_end && success) {
                    m_includes.push_back(current);
                }
                state = include_scan_state::idle;
                break;
            }

        } while (!lex.end_of_source());

        return success;
    }

} // namespace tavros::renderer

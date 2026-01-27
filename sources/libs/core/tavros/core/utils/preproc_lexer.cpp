#include <tavros/core/utils/preproc_lexer.hpp>

#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <cctype>

namespace
{
	template <class CharT>
	bool is_whitespace(CharT c) noexcept
	{
		return std::isspace(static_cast<int>(c)) != 0;
	}

	template <class CharT>
	bool is_h_whitespace(CharT c) noexcept
	{
		return is_whitespace(c) && c != '\n';
	}

	template <class CharT>
	bool is_start_identifier(CharT c) noexcept
	{
		return std::isalpha(static_cast<int>(c)) != 0 || c == '_';
	}

	template <class CharT>
	bool is_part_identifier(CharT c) noexcept
	{
		return is_start_identifier(c) || std::isdigit(static_cast<int>(c)) != 0;
	}

	template <class CharT>
	bool is_start_number(CharT c) noexcept
	{
		return std::isdigit(static_cast<int>(c)) != 0;
	}

	template <class CharT>
	bool is_part_number(CharT c) noexcept
	{
		return is_part_identifier(c);
	}
}

namespace tavros::core
{

	using tt = preproc_token::token_type;

	preproc_lexer::preproc_lexer(const char* begin, const char* end) noexcept
		: m_begin(begin)
		, m_end(end)
		, m_forward(begin)
		, m_line_begin(begin)
		, m_line_end(begin)
		, m_line(1)
		, m_column(1)
		, m_in_include_directive(false)
	{
		while (m_line_end < m_end && *m_line_end != '\n') {
			++m_line_end;
		}
	}

	preproc_token preproc_lexer::next_token() noexcept
	{
		// Placeholder implementation: returns end_of_file token
		if (end_of_source()) {
			return {tt::end_of_file, m_end, m_end, m_line_begin, m_line_end, m_line, m_column};
		}

		if (m_in_include_directive) {
			return scan_header_name();
		}

		const char* end = m_end;

		while (m_forward < end) {
			skip_whitespace();

			const auto* beg = m_forward;

			switch (*m_forward)
			{
			case '#':
				// Preprocessor directive or punctuation
				++m_forward;
				++m_column;
				skip_h_whitespace();
				
				if (is_start_identifier(*m_forward)) {
					beg = m_forward;
					++m_forward;
					++m_column;

					while (m_forward < end && is_part_identifier(*m_forward)) {
						++m_forward;
						++m_column;
					}

					auto directive_len = static_cast<size_t>(m_forward - beg);

					constexpr const char include_name[] = "include";
					constexpr size_t include_name_len = sizeof(include_name) - 1;

					if (include_name_len == directive_len && strncmp(beg, include_name, directive_len) == 0) {
						m_in_include_directive = true;
					}

					return {tt::directive, beg, m_forward, m_line_begin, m_line_end, m_line, m_column};
				}
				return {tt::punctuation, beg, m_forward, m_line_begin, m_line_end, m_line, m_column};

			case '/':
				++m_forward;
				++m_column;

				if (m_forward < end) {
					if (*m_forward == '*') {
						// Multi-line comment
						++m_forward;
						++m_column;

						skip_multi_line_comment();
					} else if (*m_forward == '/') {
						// Single-line comment
						++m_forward;
						++m_column;

						skip_single_line_comment();
					} else {
						// Just a division operator
						return {tt::punctuation, beg, m_forward, m_line_begin, m_line_end, m_line, m_column};
					}
				} else {
					// Just a division operator at end of source
					return {tt::punctuation, beg, m_forward, m_line_begin, m_line_end, m_line, m_column};
				}

				break;

			case '"':
				[[fallthrough]];
			case '\'':
				// String literal or character literal
				return scan_string_literal();
				
			default:
				
				beg = m_forward;

				// Identifier
				if (is_start_identifier(*m_forward)) {
					auto col = m_column;

					++m_forward;
					++m_column;
					while (m_forward < end && is_part_identifier(*m_forward)) {
						++m_forward;
						++m_column;
					}

					return {tt::identifier, beg, m_forward, m_line_begin, m_line_end, m_line, col};
				}

				// Number literal
				if (is_start_number(*m_forward)) {
					auto col = m_column;

					++m_forward;
					++m_column;
					while (m_forward < end && is_part_number(*m_forward)) {
						++m_forward;
						++m_column;
					}

					return {tt::number, beg, m_forward, m_line_begin, m_line_end, m_line, col};
				}

				if (m_forward == m_end) {
					break;
				}

				++m_forward;
				++m_column;
				return {tt::punctuation, beg, m_forward, m_line_begin, m_line_end, m_line, m_column};
			}
		}

		return {tt::end_of_file, m_end, m_end, m_line_begin, m_line_end, m_line, m_column};
	}

	bool preproc_lexer::end_of_source() const noexcept
	{
		return m_forward >= m_end;
	}

	preproc_token preproc_lexer::scan_string_literal()
	{
		TAV_ASSERT(*m_forward == '\'' || *m_forward == '"');

		auto ln = m_line;
		auto col = m_column;

		// String literal or character literal
		const char quote_char = *m_forward;
		++m_forward;
		++m_column;
		const auto* beg = m_forward;
		const char* end = m_end;

		while (m_forward < end && *m_forward != '\n' && *m_forward != quote_char) {
			if (*m_forward == '\\') {
				// Escape sequence
				++m_forward;
				++m_column;
				if (m_forward < end) {
					++m_forward;
					++m_column;
				} else {
					// Error: incomplete escape sequence
					return {tt::error, beg, m_forward, m_line_begin, m_line_end, m_line, m_column, "Incomplete escape sequence"};
				}
			} else {
				++m_forward;
				++m_column;
			}
		}

		if (m_forward == end) {
			if (quote_char == '"') {
				return {tt::error, beg, m_forward, m_line_begin, m_line_end, m_line, m_column, "Unexpected end of file in string literal"};
			}
			return {tt::error, beg, m_forward, m_line_begin, m_line_end, m_line, m_column, "Unexpected end of file in character literal"};
		}

		if (*m_forward == quote_char) {
			// Closing quote found
			const auto* end_of_str = m_forward;
			++m_forward;
			++m_column;

			if (quote_char == '"') {
				return {tt::string_literal, beg, end_of_str, m_line_begin, m_line_end, ln, col};
			}
			return {tt::character_literal, beg, end_of_str, m_line_begin, m_line_end, ln, col};
		}

		// Error: unterminated string/character literal
		if (quote_char == '"') {
			return {tt::error, beg - 1, m_forward, m_line_begin, m_line_end, m_line, m_column, "Unterminated string literal"};
		}
		return {tt::error, beg - 1, m_forward, m_line_begin, m_line_end, m_line, m_column, "Unterminated character literal"};
	}

	preproc_token preproc_lexer::scan_header_name()
	{
		const auto* end = m_end;
		skip_h_whitespace();

		if (*m_forward == '<' || *m_forward == '"') {
			// Header name
			++m_forward;
			++m_column;
			const auto* beg = m_forward;
			while (m_forward < end && *m_forward != '\n' && *m_forward != '"' && *m_forward != '>') {
				++m_forward;
				++m_column;
			}

			if (m_forward == end) {
				return { tt::error, beg - 1, m_forward, m_line_begin, m_line_end, m_line, m_column, "Unexpected end of file in header name" };
			}

			if (*m_forward == '\n') {
				return { tt::error, beg - 1, m_forward, m_line_begin, m_line_end, m_line, m_column, "Unexpected end of line in header name" };
			}

			if (*m_forward == '"' || *m_forward == '>') {
				const auto* end_header_name = m_forward;

				// Closing quote found
				++m_forward;
				++m_column;

				// Skip to end of line
				while (m_forward < end && is_h_whitespace(*m_forward)) {
					++m_forward;
					++m_column;
				}

				if (m_forward != end && *m_forward != '\n') {
					// Error: unexpected characters after header name
					auto ln = m_line;
					auto col = m_column;
					skip_to_end_line();

					return { tt::error, beg - 1, m_forward, m_line_begin, m_line_end, ln, col, "Unexpected characters after header name" };
				}

				m_in_include_directive = false;

				return {tt::header_name, beg, end_header_name, m_line_begin, m_line_end, m_line, m_column};
			}

			TAV_UNREACHABLE();
		}

		// Error: expected header name
		auto ln = m_line;
		auto col = m_column;
		const auto* beg = m_forward;

		skip_to_end_line();

		return {tt::error, beg, m_forward, m_line_begin, m_line_end, ln, col, "Expected header name after #include"};
	}

	void preproc_lexer::skip_whitespace()
	{
		while (m_forward < m_end && is_whitespace(*m_forward)) {
			if (*m_forward == '\n') {
				process_new_line();
			} else {
				++m_column;
				++m_forward;
			}
		}
	}

	void preproc_lexer::skip_h_whitespace()
	{
		while (m_forward < m_end && is_h_whitespace(*m_forward)) {
			++m_column;
			++m_forward;
		}
	}

	void preproc_lexer::skip_to_end_line()
	{
		while (m_forward < m_end && *m_forward != '\n') {
			++m_forward;
			++m_column;
		}
		m_in_include_directive = false;
	}

	void preproc_lexer::skip_multi_line_comment()
	{
		while (m_forward < m_end) {
			if (*m_forward == '*' && (m_forward + 1) < m_end && *(m_forward + 1) == '/') {
				// End of comment found
				m_forward += 2;
				m_column += 2;
				return;
			} else {
				if (*m_forward == '\n') {
					process_new_line();
				} else {
					++m_column;
					++m_forward;
				}
			}
		}
	}

	void preproc_lexer::skip_single_line_comment()
	{
		while (m_forward < m_end && *m_forward != '\n') {
			++m_forward;
			++m_column;
		}
	}

	void preproc_lexer::process_new_line()
	{
		TAV_ASSERT(m_forward < m_end && *m_forward == '\n');
		++m_line;
		m_column = 1;
		++m_forward;
		m_line_begin = m_forward;
		m_in_include_directive = false;
		m_line_end = m_line_begin;

		while (m_line_end < m_end && *m_line_end != '\n') {
			++m_line_end;
		}
	}

}

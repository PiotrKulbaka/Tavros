#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::core
{

	class preproc_lexer;

	/**
	 * @brief Represents a single token produced by the preprocessor lexer.
	 *
	 * `preproc_token` is a lightweight, non-owning view into the original source
	 * code. It stores the token type, its text range, and source location
	 * (line and column). No dynamic memory allocations are performed.
	 *
	 * All returned string views reference the original source buffer, which must
	 * remain valid while the token is in use.
	 */
	class preproc_token
	{
	public:

		/**
		 * @brief Enumeration of all supported preprocessor token kinds.
		 */
		enum class token_type
		{
			directive,			/// Preprocessor directive (e.g. #include, #define)
			header_name,        /// Header name in include directive
			identifier,         /// Identifier
			number,             /// Numeric literal
			punctuation,        /// Punctuation character
			string_literal,     /// String literal
			character_literal,  /// Character literal
			error,              /// Lexing error
			end_of_file,        /// End of source
		};

	public:
		/**
		 * @brief Returns the token kind.
		 */
		token_type type() const noexcept
		{
			return m_type;
		}

		/**
		 * @brief Returns the token text.
		 *
		 * The returned text depends on the token type:
		 * - For string and character literals, the surrounding quotes (`"` or `'`)
		 *   are not included.
		 * - For header names, the surrounding delimiters (`"` or `< >`) are not included.
		 *
		 * The returned view always refers to the original source buffer.
		 */
		string_view token() const noexcept
		{
			return string_view(m_begin, static_cast<size_t>(m_end - m_begin));
		}

		/**
		 * @brief Returns the full source line containing the token.
		 */
		string_view line() const noexcept
		{
			return string_view(m_begin - (m_column - 1), m_column - 1 + static_cast<size_t>(m_end - m_begin));
		}

		/**
		 * @brief Returns the 1-based line number of the token.
		 */
		size_t line_number() const noexcept
		{
			return m_line;
		}

		/**
		 * @brief Returns the 1-based column number of the token.
		 */
		size_t column_number() const noexcept
		{
			return m_column;
		}

		/**
		 * @brief Returns an error description if the token represents an error.
		 */
		string_view error_string() const noexcept
		{
			return m_error_str;
		}

	private:
		preproc_token(
			token_type type, 
			const char* token_begin, 
			const char* token_end, 
			const char* line_begin, 
			const char* line_end, 
			size_t line, 
			size_t column, 
			string_view error_str = {}
		) noexcept
			: m_type(type)
			, m_begin(token_begin)
			, m_end(token_end)
			, m_line_begin(line_begin)
			, m_line_end(line_end)
			, m_line(line)
			, m_column(column)
			, m_error_str(error_str)
		{
		}

	private:
		token_type  m_type;
		const char* m_begin;
		const char* m_end;
		const char* m_line_begin;
		const char* m_line_end;
		size_t	    m_line;
		size_t	    m_column;
		string_view m_error_str;

		friend class preproc_lexer;
	};

}

#pragma once

#include <tavros/core/utils/preproc_token.hpp>

namespace tavros::core
{

	/**
	 * @brief A lexer for the GLSL preprocessor.
	 *
	 * `preproc_lexer` scans a range of source code and produces individual
	 * preprocessor tokens, including directives, identifiers, numbers,
	 * string/character literals, punctuation, and header names. It handles
	 * whitespace, comments, and tracks line/column positions for each token.
	 *
	 * @note The lexer does not own the memory it scans and performs no dynamic
	 *       memory allocations. It is the caller's responsibility to ensure
	 *       that the source memory remains valid for the lifetime of the lexer.
	 */
	class preproc_lexer
	{
	public:
		/**
		 * @brief Constructs a lexer for a given source range.
		 * @param begin Pointer to the first character of the source code.
		 * @param end   Pointer one past the last character of the source code.
		 */
		preproc_lexer(const char* begin, const char* end) noexcept;

		/**
		 * @brief Default destructor. Nothing to release since no dynamic memory is used.
		 */
		~preproc_lexer() = default;

		/**
		 * @brief Returns the next token from the source code.
		 * @return The next `preproc_token`, or `end_of_file` if at the end.
		 */
		[[nodiscard]] preproc_token next_token() noexcept;

		/**
		 * @brief Checks if the lexer has reached the end of the source.
		 * @return `true` if fully lexed, `false` otherwise.
		 */
		bool end_of_source() const noexcept;

	private:
		// Also scan character literal
		preproc_token scan_string_literal();
		preproc_token scan_header_name();

		void skip_whitespace();
		void skip_h_whitespace();
		void skip_to_end_line();
		void skip_multi_line_comment();
		void skip_single_line_comment();

		void process_new_line();

	private:
		const char* m_begin;
		const char* m_end;
		const char* m_forward;
		const char* m_line_begin;
		const char* m_line_end;
		size_t m_line;
		size_t m_column;
		bool m_in_include_directive = false;
	};

}

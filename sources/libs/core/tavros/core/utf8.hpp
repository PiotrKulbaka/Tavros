#pragma once

#include <tavros/core/types.hpp>

namespace tavros::core
{

    /**
     * @brief Extracts a single Unicode code point from a UTF-8 encoded byte sequence.
     *
     * Decodes the next UTF-8 sequence starting at `text` and returns the corresponding
     * Unicode code point as a `char32`. Advances the output pointer `*out` to the
     * position immediately after the decoded sequence.
     *
     * The function performs strict validation of UTF-8 byte patterns:
     * - Invalid or truncated sequences return the replacement character (`U+FFFD`).
     * - Overlong encodings and invalid code points (e.g., surrogate halves) are rejected.
     * - If `text == end`, the function returns `0` and sets `*out = end`.
     *
     * @param text Pointer to the start of the UTF-8 sequence.
     * @param end  Pointer to one past the last valid byte in the buffer.
     * @param out  Output pointer that receives the position after the parsed sequence.
     * @return The decoded Unicode code point, or 0xFFFD (replacement character) on error.
     */
    char32 extract_utf8_codepoint(const char* text, const char* end, const char** out);

    /**
     * @brief Check whether a codepoint is a Unicode line break character.
     *
     * Recognizes:
     *  - U+000A LINE FEED
     *  - U+2028 LINE SEPARATOR
     *  - U+2029 PARAGRAPH SEPARATOR
     *
     * @param cp Unicode codepoint.
     * @return True if the codepoint represents a line break.
     */
    bool is_unicode_line_break(char32 cp) noexcept;

    /**
     * @brief Check whether a codepoint has the Unicode White_Space property.
     *
     * Includes ASCII whitespace characters and all Unicode space separators
     * defined by the Unicode White_Space property.
     *
     * @param cp Unicode codepoint.
     * @return True if the codepoint is considered whitespace.
     */
    bool is_unicode_whitespace(char32 cp) noexcept;

} // namespace tavros::core

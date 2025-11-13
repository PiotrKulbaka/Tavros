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

} // namespace tavros::core

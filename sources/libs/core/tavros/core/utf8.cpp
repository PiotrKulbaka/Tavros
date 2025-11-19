#include <tavros/core/utf8.hpp>

#include <tavros/core/debug/assert.hpp>

namespace tavros::core
{

    char32 extract_utf8_codepoint(const char* text, const char* end, const char** out)
    {
        constexpr char32 rc = 0xFFFD; // REPLACEMENT CHARACTER
        TAV_ASSERT(text);
        TAV_ASSERT(end);
        TAV_ASSERT(out);
        TAV_ASSERT(text <= end);

        if (text + 0 == end) {
            *out = end;
            return 0;
        }

        char32 c0 = static_cast<uint8>(text[0]);
        if (!(c0 & 0x80)) {
            *out = text + 1;
            return c0;
        }

        if (text + 1 == end) {
            *out = end;
            return rc;
        }

        char32 c1 = static_cast<uint8>(text[1]);
        if ((c1 & 0xC0) != 0x80) {
            *out = text + 1;
            return rc;
        }

        if ((c0 & 0xE0) == 0xC0) {
            char32 cp = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
            *out = text + 2;
            return cp < 0x80 ? rc : cp;
        }

        if (text + 2 == end) {
            *out = end;
            return rc;
        }

        char32 c2 = static_cast<uint8>(text[2]);
        if ((c2 & 0xC0) != 0x80) {
            *out = text + 1;
            return rc;
        }

        if ((c0 & 0xF0) == 0xE0) {
            char32 cp = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
            *out = text + 3;
            return cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF) ? rc : cp;
        }

        if (text + 3 == end) {
            *out = end;
            return rc;
        }

        char32 c3 = static_cast<uint8>(text[3]);
        if ((c3 & 0xC0) != 0x80) {
            *out = text + 1;
            return rc;
        }

        if ((c0 & 0xF8) == 0xF0) {
            char32 cp = ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
            *out = text + 4;
            return cp < 0x10000 || cp > 0x10FFFF ? rc : cp;
        }

        *out = text + 1;
        return rc;
    }

} // namespace tavros::core

#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/core/geometry/aabb2.hpp>
#include <tavros/core/math/vec2.hpp>
#include <tavros/text/font/font.hpp>
#include <tavros/core/archetype.hpp>

namespace tavros::text
{

    struct glyph_c
    {
        const font* font = nullptr;      /// Pointer to the font used for this glyph.
        char32      codepoint = 0;       /// Unicode codepoint.
        float       glyph_size = 0.0f;   /// Glyph rendering size.
        float       step_x = 0.0f;       /// Scaled horizontal step (advance + kerning)
        bool        is_space = false;    /// True if the glyph is whitespace.
        bool        is_new_line = false; /// True if the glyph is whitespace.
    };

    struct atlas_rect_c : public atlas_rect
    {
    };
    struct rect_layout_c : public geometry::aabb2
    {
    };
    struct rect_bounds_c : public geometry::aabb2
    {
    };
    struct position_c : public math::vec2
    {
    };


    struct text_line_c
    {
        /// Index of the line's first glyph in the original text
        uint32 first_glyph_index = 0;

        /// Number of glyphs in the line
        uint32 glyph_count = 0;

        /// Number of whitespace characters between words in a line, excluding leading and trailing whitespaces
        uint32 inter_word_whitespace_count = 0;

        /// Max available width for the line (line_wrap_width)
        float max_line_width = 0.0f;

        /// Width of the line excluding trailing_whitespace_width (sum of step_x)
        float content_width = 0.0f;

        /// Width of trailing whitespace characters (sum of step_x)
        float trailing_whitespace_width = 0.0f;

        /// Max ascent of the line
        float ascent_y = 0.0f;

        /// Max descent of the line
        float descent_y = 0.0f;

        /// Line is terminated by a newline character, or the end of text
        bool ends_with_newline = false;
    };

} // namespace tavros::text

#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    /**
     * @brief Text line layout component.
     *
     * Describes a contiguous sequence of glyphs forming a single line.
     * Stores index range and aggregated vertical/horizontal metrics
     * required for layout and alignment.
     *
     * Vertical metrics represent maximal ascent/descent among glyphs
     * in the line and are already scaled.
     */
    struct text_line_c
    {
        /// Index of the first glyph of this line in the glyph sequence.
        uint32 first_glyph_index = 0;

        /// Number of glyphs belonging to this line.
        uint32 glyph_count = 0;

        /// Maximum ascent among all glyphs in the line.
        float ascent_y = 0.0f;

        /// Maximum descent among all glyphs in the line.
        float descent_y = 0.0f;

        /// Line width excluding trailing whitespace (sum of step_x).
        float width = 0.0f;
    };

} // namespace tavros::renderer

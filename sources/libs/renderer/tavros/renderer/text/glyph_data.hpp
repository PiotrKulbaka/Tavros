#pragma once

#include <tavros/renderer/text/font/font.hpp>

namespace tavros::renderer
{
    /**
     * @brief Glyph layout component.
     *
     * Stores font-related metrics and layout data required to position
     * a single glyph within a text run.
     *
     * All metric values are already scaled to the requested font size.
     * Vertical metrics follow a baseline-oriented coordinate system:
     * ascent is typically negative, descent is typically positive.
     */
    struct glyph_c
    {
        /// Font used to rasterize this glyph.
        const font* font = nullptr;

        /// Unicode codepoint represented by this glyph.
        char32 codepoint = 0;

        /// Glyph size in pixels.
        float size = 0.0f;

        /// Horizontal advance to the next glyph (including kerning), in pixels.
        float step_x = 0.0f;

        /// Vertical ascent from baseline to glyph top (usually negative).
        float ascent_y = 0.0f;

        /// Vertical descent from baseline to glyph bottom (usually positive).
        float descent_y = 0.0f;
    };

} // namespace tavros::renderer

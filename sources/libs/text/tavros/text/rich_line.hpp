#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/utf8.hpp>
#include <tavros/core/math.hpp>
#include <tavros/core/geometry/aabb2.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/text/font/font.hpp>

#include <cwctype>

namespace tavros::text
{

    /**
     * @brief Base glyph information used by the rich text line builder.
     *
     * Stores all font-dependent metrics and layout-related fields for a single glyph.
     * These values are updated by style and font assignment functions.
     */
    struct glyph_data_base
    {
        char32 codepoint = 0;                /// Unicode codepoint.
        bool   is_space = false;             /// True if the glyph is whitespace.
        bool   is_new_line = false;          /// True if the glyph is new line.

        const font*       font = nullptr;    /// Pointer to the font used for this glyph.
        font::glyph_index idx = 0;           /// Index of the glyph in the font.
        float             glyph_size = 0.0f; /// Glyph rendering size (scale).

        atlas_rect rect;                     /// Texture atlas rectangle of the glyph image.
        math::vec2 bearing;                  /// Glyph bearing scaled by glyph_size.
        float      advance_x = 0.0f;         /// Horizontal advance (scaled).
        float      kern_x = 0.0f;            /// Horizontal kerning to the next glyph (scaled).

        geometry::aabb2 layout;              /// Final layout position relative to the baseline.
        geometry::aabb2 bounds;              /// Bounding box of the rendered glyph (for selecting).
    };

    /**
     * @brief Combines base glyph data with user-defined per-glyph parameters.
     *
     * @tparam GlyphParams  User-defined parameter type stored alongside each glyph.
     */
    template<class GlyphParams>
    struct glyph_data_param
    {
        glyph_data_base base;   /// Basic glyph metrics and layout information.
        GlyphParams     params; /// User-defined glyph parameters.
    };

    /**
     * @brief Specialization for glyphs without additional parameters.
     */
    template<>
    struct glyph_data_param<void>
    {
        glyph_data_base base; /// Basic glyph metrics only.
    };

    /**
     * @brief Holds and manages a sequence of glyphs representing a styled text line.
     *
     * The class stores raw glyphs with metrics and dynamically updates them when font,
     * size, or style ranges are modified. No layout or line breaking is performed here —
     * this class only prepares glyph metrics and kerning before layout.
     *
     * @tparam GlyphParams  Optional per-glyph extension data.
     */
    template<class GlyphParams = void>
    class rich_line
    {
    public:
        using glyph_data = glyph_data_param<GlyphParams>;

    public:
        rich_line() noexcept = default;

        ~rich_line() noexcept = default;

        /**
         * @brief Initializes glyphs from a Unicode text string and applies a uniform font and size.
         *
         * Clears existing glyphs, creates one glyph per codepoint, determines whitespace flags,
         * and applies the specified font and size to the entire range.
         *
         * @param text  UTF-8 string view containing the text.
         * @param fnt   Font used for glyph lookup and metrics.
         * @param size  Rendering size of all glyphs.
         */
        void set_text(core::string_view text, const font* fnt, float size)
        {
            TAV_ASSERT(fnt != nullptr);

            m_glyphs.clear(); // clear old data
            m_glyphs.reserve(text.length());

            const char* it = text.data();
            const char* end = text.data() + text.length();

            char32 cp = 0;
            while (0 != (cp = core::extract_utf8_codepoint(it, end, &it))) {
                glyph_data g;
                g.base.codepoint = cp;
                g.base.is_space = static_cast<bool>(std::iswspace(cp));
                g.base.is_new_line = cp == static_cast<char32>('\n');

                m_glyphs.push_back(g);
            }

            set_style(0, m_glyphs.size(), fnt, size);
        }

        /**
         * @brief Applies font and size to a glyph range and updates metrics and kerning.
         *
         * For each glyph in the specified range, computes glyph index, bearings, advances,
         * and kerning to the next glyph. Kerning is applied both within the style range and,
         * if possible, across range boundaries when the adjacent glyph uses the same font.
         *
         * @param begin  Start index (inclusive).
         * @param end    End index (exclusive).
         * @param fnt    Font to apply.
         * @param size   Glyph rendering size.
         */
        void set_style(size_t begin, size_t end, const font* fnt, float size)
        {
            TAV_ASSERT(fnt != nullptr);
            TAV_ASSERT(begin <= m_glyphs.size());
            TAV_ASSERT(end <= m_glyphs.size());
            TAV_ASSERT(begin <= end);

            for (auto i = begin; i < end; ++i) {
                auto& g = m_glyphs[i].base;
                auto  idx = fnt->find_glyph(g.codepoint);
                auto& info = fnt->get_glyph_info(idx);

                g.font = fnt;
                g.idx = idx;
                g.glyph_size = size;
                g.rect = info.entry;
                g.bearing = info.metrics.bearing * size;
                g.advance_x = info.metrics.advance.x * size;

                auto next = i + 1;
                if (next < end || (next < m_glyphs.size() && fnt == m_glyphs[next].base.font)) {
                    auto next_idx = fnt->find_glyph(m_glyphs[next].base.codepoint);
                    g.kern_x = fnt->get_kerning(g.idx, next_idx) * g.glyph_size;
                } else {
                    g.kern_x = 0.0f;
                }
            }
        }

        /**
         * @brief Changes the font for a glyph range without modifying glyph size.
         *
         * Updates the base font-dependent metrics (bearing, advance, kerning) according to
         * the new font. Kerning applies within the range and beyond it if the adjacent glyph
         * uses the same font.
         *
         * @param begin  Start index (inclusive).
         * @param end    End index (exclusive).
         * @param fnt    New font pointer.
         */
        void set_font(size_t begin, size_t end, font* fnt)
        {
            TAV_ASSERT(fnt != nullptr);
            TAV_ASSERT(begin <= m_glyphs.size());
            TAV_ASSERT(end <= m_glyphs.size());
            TAV_ASSERT(begin <= end);

            for (auto i = begin; i < end; ++i) {
                auto& g = m_glyphs[i].base;
                auto  idx = fnt->find_glyph(g.codepoint);
                auto& info = fnt->get_glyph_info(idx);

                g.font = fnt;
                g.idx = idx;
                g.rect = info.entry;
                g.bearing = info.metrics.bearing * g.glyph_size;
                g.advance_x = info.metrics.advance.x * g.glyph_size;

                auto next = i + 1;
                if (next < end || (next < m_glyphs.size() && fnt == m_glyphs[next].base.font)) {
                    auto next_idx = fnt->find_glyph(m_glyphs[next].base.codepoint);
                    g.kern_x = fnt->get_kerning(g.idx, next_idx) * g.glyph_size;
                } else {
                    g.kern_x = 0.0f;
                }
            }
        }

        /**
         * @brief Changes glyph size for a range while keeping the previously assigned font.
         *
         * Recomputes bearings, advances, and kerning using the stored font and scaled metrics.
         * Kerning is updated only when the next glyph uses the same font.
         *
         * @param begin  Start index (inclusive).
         * @param end    End index (exclusive).
         * @param size   New glyph size.
         */
        void set_font_size(size_t begin, size_t end, float size)
        {
            TAV_ASSERT(begin <= m_glyphs.size());
            TAV_ASSERT(end <= m_glyphs.size());
            TAV_ASSERT(begin <= end);

            for (auto i = begin; i < end; ++i) {
                auto& g = m_glyphs[i].base;
                auto& info = g.font->get_glyph_info(g.idx);

                g.glyph_size = size;
                g.bearing = info.metrics.bearing * size;
                g.advance_x = info.metrics.advance.x * size;
                auto next = i + 1;
                if (next < m_glyphs.size() && g.font == m_glyphs[next].base.font) {
                    g.kern_x = g.font->get_kerning(g.idx, m_glyphs[next].base.idx) * size;
                } else {
                    g.kern_x = 0.0f;
                }
            }
        }

        /**
         * @brief Returns a mutable span over the internal glyph storage.
         *
         * Allows external systems (such as text layout components) to read and write layout fields.
         *
         * @return Mutable span of glyph_data.
         */
        core::buffer_span<glyph_data> glyphs() noexcept
        {
            return m_glyphs;
        }

    private:
        core::vector<glyph_data> m_glyphs;
    };

} // namespace tavros::text
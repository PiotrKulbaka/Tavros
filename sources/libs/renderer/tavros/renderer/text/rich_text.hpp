#pragma once

#include <tavros/renderer/text/font/font.hpp>
#include <tavros/renderer/text/text_layouter.hpp>
#include <tavros/renderer/text/text_builder.hpp>
#include <tavros/renderer/components/all.hpp>

namespace tavros::renderer
{

    /**
     * @brief Per-glyph rendering style.
     */
    struct glyph_style_c
    {
        /// @brief Glyph fill color.
        math::rgba8 fill_color;

        /// @brief Glyph outline color.
        math::rgba8 outline_color;
    };

    /// Text archetype containing glyph data and layout information.
    using text_archetype = core::basic_archetype<glyph_c, atlas_rect_t, rect_layout_c, position2d_c, glyph_style_c>;

    /**
     * @brief Rich text container with formatting and layout support.
     */
    class rich_text
    {
    public:
        /**
         * @brief Constructs a rich text object.
         * @param initial_font Initial font used for appended text.
         * @param initial_font_size Initial font size in pixels.
         */
        rich_text(font_ref initial_font = {}, float initial_font_size = 1.0f) noexcept;

        /** @brief Destroys the rich text object. */
        ~rich_text() noexcept = default;

        /** @brief Sets the current font for subsequently appended text. */
        void set_font(font_ref fnt) noexcept;

        /** @brief Sets the current font size in pixels. */
        void set_font_size(float size_px) noexcept;

        /** @brief Sets the current glyph fill color. */
        void set_fill_color(math::rgba8 color) noexcept;

        /** @brief Sets the current glyph outline color. */
        void set_outline_color(math::rgba8 color) noexcept;

        /** @brief Sets the line spacing multiplier. */
        void set_line_spacing(float spacing) noexcept;

        /** @brief Sets the maximum line width used for word wrapping. */
        void set_line_wrap_width(float width) noexcept;

        /** @brief Sets the horizontal text alignment. */
        void set_text_align(text_align align) noexcept;

        /**
         * @brief Appends text using the current formatting state.
         * @param str UTF-8 text to append.
         */
        void append_text(core::string_view str);

        /** @brief Removes all text and formatting data. */
        void clear() noexcept;

        /** @brief Releases unused memory. */
        void shrink_to_fit();

        /**
         * @brief Computes the text layout if it is dirty.
         * @return Bounding box of the laid out text.
         */
        geometry::aabb2 layout() noexcept;

        /**
         * @brief Returns the laid out text archetype.
         * @return Reference to the internal text archetype.
         */
        const text_archetype& text() noexcept;

    private:
        text_archetype  m_text;
        bool            m_dirty;
        font_ref        m_cur_font;
        float           m_cur_font_size;
        math::rgba8     m_cur_fill_color;
        math::rgba8     m_cur_outline_color;
        float           m_line_wrap_width;
        text_align      m_text_align;
        float           m_line_spacing;
        geometry::aabb2 m_aabb;
    };

} // namespace tavros::renderer

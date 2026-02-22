#pragma once

#include <tavros/core/containers/map.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/utils/string_string_view_comparator.hpp>
#include <tavros/renderer/text/font/font.hpp>
#include <tavros/renderer/text/font/font_atlas.hpp>
#include <tavros/renderer/text/font/font_data_provider.hpp>

namespace tavros::renderer
{

    /**
     * @brief Centralized font storage and atlas management.
     *
     * The font_library is responsible for:
     * - Loading fonts via a font_data_provider
     * - Owning font instances and providing name-based access
     * - Tracking atlas invalidation state
     * - Rebuilding the font atlas when required
     *
     * The class is non-copyable and owns all loaded font resources.
     */
    class font_library : core::noncopyable
    {
    public:
        /**
         * @brief Lightweight non-owning view of a loaded font.
         *
         * Used to expose font metadata without transferring ownership.
         * The lifetime of the referenced font is guaranteed by font_library.
         */
        struct font_view
        {
            core::string_view   name;
            core::raw_ptr<font> font;
        };

    public:
        /**
         * @brief Constructs a font library with a given data provider.
         *
         * @param dp Unique ownership of the font data provider.
         */
        font_library(core::unique_ptr<font_data_provider> dp) noexcept;

        /**
         * @brief Destroys the font library and all owned fonts.
         */
        ~font_library() noexcept = default;

        /**
         * @brief Loads a font from a file and registers it under the given name.
         *
         * If a font with the same name already exists, the behavior is undefined
         * unless explicitly handled in the implementation.
         *
         * @param path Path to the font source.
         * @param name Logical name used to access the font.
         */
        void load(core::string_view path, core::string_view name);

        /**
         * @brief Attempts to retrieve a font by name.
         *
         * @param name Font identifier.
         * @return Pointer to the font if found, nullptr otherwise.
         */
        font* try_get(core::string_view name) const noexcept;

        /**
         * @brief Checks whether the font atlas needs to be recreated.
         *
         * Typically becomes true after loading new fonts.
         *
         * @return True if atlas recreation is required.
         */
        bool need_to_recreate_atlas() const noexcept;

        /**
         * @brief Invalidates the old atlas and builds a new one.
         *
         * This function recreates the atlas for all registered fonts, updates UV coordinates
         * in these fonts, and writes the resulting single-channel (8-bit) grayscale/SDF bitmap.
         *
         * @param glyph_scale_pix Glyph size in pixels used during baking.
         * @param glyph_sdf_pad_pix Amount of SDF padding around each glyph, in pixels.
         *
         * @return Pixel data of the newly baked atlas.
         */
        font_atlas::atlas_pixels invalidate_old_and_bake_new_atlas(float glyph_scale_pix, float glyph_sdf_pad_pix);

        /**
         * @brief Returns a view of all loaded fonts.
         *
         * The returned buffer is valid as long as the font_library instance exists
         * and no structural modifications are made to the font set.
         *
         * @return Buffer view of font_view entries.
         */
        core::buffer_view<font_view> fonts() const noexcept;

    private:
        using data_provider_type = core::unique_ptr<font_data_provider>;
        using map_type = core::map<core::string, core::unique_ptr<font>, core::string_string_view_comparator>;
        using font_vector_type = core::vector<font_view>;

        data_provider_type m_data_provider;
        font_vector_type   m_vector;
        map_type           m_fonts;
        font_atlas         m_atlas;
    };

} // namespace tavros::renderer
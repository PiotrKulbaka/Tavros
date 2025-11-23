#pragma once

#include <tavros/core/types.hpp>

namespace tavros::text
{

    /**
     * @brief Atlas-related data for a glyph.
     *
     * Includes texture coordinate bounds and SDF-specific padding.
     */
    struct atlas_rect
    {
        /// Rectangle of the glyph in the atlas.
        uint16 left = 0;
        uint16 top = 0;
        uint16 right = 0;
        uint16 bottom = 0;
    };

} // namespace tavros::text

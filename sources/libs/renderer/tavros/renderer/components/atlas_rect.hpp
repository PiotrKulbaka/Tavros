#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    /**
     * @brief Atlas-related data.
     *
     * Includes texture coordinate bounds.
     */
    struct atlas_rect_t
    {
        /// Rectangle of the glyph in the atlas.
        uint16 left = 0;
        uint16 top = 0;
        uint16 width = 0;
        uint16 height = 0;
    };

} // namespace tavros::renderer

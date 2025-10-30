#pragma once

#include <tavros/ui/base.hpp>
#include <tavros/core/memory/buffer_view.hpp>

namespace tavros::ui
{

    class font
    {
    public:
        font();
        ~font();

        bool init(core::dynamic_buffer<uint8> font_data);
        void shutdown();

        bool is_init();

        uint32 get_glyph_index(uint32 codepoint);


    private:
    };

} // namespace tavros::ui

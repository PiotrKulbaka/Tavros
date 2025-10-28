#pragma once

#include <tavros/resources/resource.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/memory/dynamic_buffer.hpp>
#include <tavros/core/memory/buffer_view.hpp>

namespace app
{

    class image_codec : tavros::core::noncopyable
    {
    public:
        struct pixels_view
        {
            uint32       width = 0;
            uint32       height = 0;
            uint32       channels = 0;
            uint32       stride = 0;
            const uint8* data = nullptr;
        };

        struct image_info
        {
            uint32 width = 0;
            uint32 height = 0;
            uint32 channels = 0;
        };

    public:
        explicit image_codec(tavros::core::allocator* alc)
            : m_buffer(alc)
        {
        }

        ~image_codec() = default;

        pixels_view decode(tavros::core::buffer_view<uint8> packed_pixels, uint32 required_channels = 4, bool y_flip = false);

        tavros::core::buffer_view<uint8> encode(const pixels_view& pixels, bool y_flip = false);

    private:
        tavros::core::dynamic_buffer<uint8> m_buffer;
    };

} // namespace app

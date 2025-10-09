#pragma once

#include <tavros/resources/resource.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/memory/resizable_buffer.hpp>

namespace app
{

    class image_loader : tavros::core::noncopyable
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
        explicit image_loader(tavros::core::allocator* alc);

        ~image_loader() = default;

        image_info decode_image_info(tavros::core::resizable_buffer<uint8>& data) const;

        pixels_view decode_image(tavros::core::resizable_buffer<uint8>& data, uint32 required_channels = 4);

    private:
        tavros::core::resizable_buffer<uint8> m_buffer;
    };

} // namespace app

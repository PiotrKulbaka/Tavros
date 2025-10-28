#include "image_codec.hpp"

#include <tavros/core/logger/logger.hpp>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace
{
    tavros::core::logger logger("image_codec");
}

namespace app
{

    image_codec::pixels_view image_codec::decode(tavros::core::buffer_view<uint8> packed_pixels, uint32 required_channels, bool y_flip)
    {
        TAV_ASSERT(1 == required_channels || 3 == required_channels || 4 == required_channels);

        static uint8_t white_pixel[4] = {255, 255, 255, 255};
        if (packed_pixels.empty()) {
            ::logger.warning("Failed to decode pixels: packed_pixels is empty. Returning fallback 1x1 white pixel");
            return {1, 1, required_channels, required_channels, white_pixel};
        }

        int info_x = 0, info_y = 0, info_channels_in_file = 0;
        if (!stbi_info_from_memory(packed_pixels.data(), packed_pixels.size(), &info_x, &info_y, &info_channels_in_file)) {
            ::logger.error("Failed to decode pixels: stbi_info_from_memory is failed");
            return {};
        }

        size_t required_size = static_cast<size_t>(info_x) * static_cast<size_t>(info_y) * static_cast<size_t>(required_channels);
        m_buffer.reserve(required_size);

        stbi_set_flip_vertically_on_load_thread(y_flip);

        int    x = 0, y = 0, channels_in_file = 0;
        uint8* pixels = stbi_load_from_memory(packed_pixels.data(), packed_pixels.size(), &x, &y, &channels_in_file, static_cast<int32>(required_channels));

        TAV_ASSERT(x == info_x);
        TAV_ASSERT(y == info_y);
        TAV_ASSERT(channels_in_file == info_channels_in_file);

        if (!pixels) {
            ::logger.error("Failed to decode pixels: stbi_load_from_memory is failed. Returning fallback 1x1 white pixel");
            return {1, 1, required_channels, required_channels, white_pixel};
        }

        m_buffer.copy_from(pixels, required_size);

        stbi_image_free(pixels);

        auto w = static_cast<uint32>(x);
        auto h = static_cast<uint32>(y);
        auto stride = static_cast<uint32>(x * required_channels);
        return {w, h, required_channels, stride, m_buffer.data()};
    }

    tavros::core::buffer_view<uint8> image_codec::encode(const image_codec::pixels_view& pixels, bool y_flip)
    {
        struct ctx_t
        {
            tavros::core::dynamic_buffer<uint8>* buf = nullptr;
            size_t                               size = 0;
        };

        ctx_t ctx{&m_buffer, 0};

        auto write_fn = [](void* ctx_ptr, void* data, int size) {
            auto* ctx = reinterpret_cast<ctx_t*>(ctx_ptr);
            auto  sz = static_cast<size_t>(size);
            ctx->buf->reserve(sz);
            ctx->buf->copy_from(data, sz);
            ctx->size = sz;
        };

        stbi_flip_vertically_on_write(y_flip);

        auto success = stbi_write_png_to_func(
            write_fn,
            &ctx,
            static_cast<int32>(pixels.width),
            static_cast<int32>(pixels.height),
            static_cast<int32>(pixels.channels),
            reinterpret_cast<const unsigned char*>(pixels.data),
            static_cast<int32>(pixels.stride)
        );

        if (!success) {
            ::logger.error("Failed to encode pixels: stbi_write_png_to_func is failed");
            return nullptr;
        }

        return {m_buffer.data(), ctx.size};
    }

} // namespace app

#include <tavros/assets/image/image.hpp>

#include <tavros/assets/image/image_codec.hpp>
#include <tavros/assets/image/image_view.hpp>
#include <tavros/core/logger/logger.hpp>

namespace
{
    tavros::assets::image::pixel_format to_pixel_format(uint32 channels) noexcept
    {
        switch (channels) {
        case 1:
            return tavros::assets::image::pixel_format::r8;
        case 2:
            return tavros::assets::image::pixel_format::rg8;
        case 3:
            return tavros::assets::image::pixel_format::rgb8;
        case 4:
            return tavros::assets::image::pixel_format::rgba8;
        default:
            return tavros::assets::image::pixel_format::none;
        }
    }

    tavros::core::logger logger("image");
} // namespace

namespace tavros::assets
{

    image image::decode(core::buffer_view<uint8> data, bool y_flip)
    {
        uint32 w = 0, h = 0, c = 0, stride = 0;
        auto   pixels = image_codec::decode(data, w, h, c, stride, 0, y_flip);

        if (pixels.empty()) {
            return {};
        }

        auto fmt = to_pixel_format(c);
        if (pixel_format::none == fmt) {
            logger.error("Invalid pixel format for channels {}", c);
            return {};
        }

        return image(std::move(pixels), w, h, fmt, stride);
    }

    core::dynamic_buffer<uint8> image::encode(image_view im, bool y_flip)
    {
        return image_codec::encode({im.data(), im.size_bytes()}, im.width(), im.height(), im.components(), im.stride(), image_codec::image_format::png, y_flip);
    }

} // namespace tavros::assets

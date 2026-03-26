#include <tavros/assets/image/image.hpp>

#include <tavros/assets/image/image_codec.hpp>
#include <tavros/assets/image/image_view.hpp>
#include <tavros/core/logger/logger.hpp>

#include <stb/stb_image_resize2.h>

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

    image image::decode(core::buffer_view<uint8> data, pixel_format required_format, bool y_flip)
    {
        uint32 w = 0, h = 0, c = 0, stride = 0;
        auto   pixels = image_codec::decode(data, w, h, c, stride, component_count(required_format), y_flip);

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

    image image::resize(image_view im, uint32 width, uint32 height, bool srgb)
    {
        TAV_ASSERT(im.valid());
        TAV_ASSERT(width > 0 && height > 0);

        auto sz = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(im.components());

        core::dynamic_buffer<uint8> dst(sz);

        stbir_pixel_layout layout;
        switch (im.format()) {
        case pixel_format::r8:
            layout = STBIR_1CHANNEL;
            break;
        case pixel_format::rg8:
            layout = STBIR_2CHANNEL;
            break;
        case pixel_format::rgb8:
            layout = STBIR_RGB;
            break;
        case pixel_format::rgba8:
            layout = STBIR_RGBA;
            break;
        default:
            TAV_UNREACHABLE();
        }

        if (srgb) {
            // sRGB resize
            auto* out = stbir_resize_uint8_srgb(
                im.data(),                     // input_pixels
                static_cast<int>(im.width()),  // input_w
                static_cast<int>(im.height()), // input_h
                static_cast<int>(im.stride()), // input_stride_in_bytes
                dst.data(),                    // output_pixels
                static_cast<int>(width),       // output_w
                static_cast<int>(height),      // output_h
                0,                             // output_stride_in_bytes
                layout                         // pixel_layout
            );

            TAV_ASSERT(out == dst.data());
            TAV_UNUSED(out);
        } else {
            // linear resize
            auto* out = stbir_resize_uint8_linear(
                im.data(),                     // input_pixels
                static_cast<int>(im.width()),  // input_w
                static_cast<int>(im.height()), // input_h
                static_cast<int>(im.stride()), // input_stride_in_bytes
                dst.data(),                    // output_pixels
                static_cast<int>(width),       // output_w
                static_cast<int>(height),      // output_h
                0,                             // output_stride_in_bytes
                layout                         // pixel_layout
            );

            TAV_ASSERT(out == dst.data());
            TAV_UNUSED(out);
        }

        return image(std::move(dst), width, height, im.format());
    }

    image image::resize(uint32 width, uint32 height, bool srgb) const
    {
        return resize(*this, width, height, srgb);
    }

} // namespace tavros::assets

#pragma once

#include <tavros/core/nonconstructable.hpp>
#include <tavros/core/memory/buffer_view.hpp>
#include <tavros/assets/image/image.hpp>

namespace tavros::assets
{

    /** @brief Encodes and decodes packed image formats (PNG, JPEG, etc.) to/from raw pixel data. */
    class image_codec : core::nonconstructable
    {
    public:
        /** @brief Supported image file formats for encoding. */
        enum class image_format
        {
            png, /// Lossless, supports alpha.
            jpg, /// Lossy, no alpha.
            tga, /// Uncompressed or RLE, optional alpha.
            bmp, /// Uncompressed or RLE, no alpha.
        };

    public:
        /**
         * @brief Decodes a compressed image into raw pixel data.
         *
         * @param data   Compressed image data (PNG, JPEG, etc.).
         * @param w      Output width in pixels.
         * @param h      Output height in pixels.
         * @param c      Output channel count.
         * @param stride Output row stride in bytes.
         * @param rc     Required channel count. 0 = use source channel count.
         * @param y_flip If true, flips the image vertically.
         *
         * @return Buffer containing raw pixel data.
         */
        static core::dynamic_buffer<uint8> decode(core::buffer_view<uint8> data, uint32& w, uint32& h, uint32& c, uint32& stride, uint32 rc = 0, bool y_flip = false);

        /**
         * @brief Encodes raw pixel data into a compressed image format.
         *
         * @param pixels Raw pixel data.
         * @param w      Image width in pixels.
         * @param h      Image height in pixels.
         * @param c      Number of color channels.
         * @param stride Row stride in bytes.
         * @param fmt    Output format (default: PNG).
         * @param y_flip If true, flips the image vertically before encoding.
         *
         * @return Buffer containing the encoded image data.
         */
        static core::dynamic_buffer<uint8> encode(core::buffer_view<uint8> pixels, uint32 w, uint32 h, uint32 c, uint32 stride, image_format fmt = image_format::png, bool y_flip = false);
    };

} // namespace tavros::assets

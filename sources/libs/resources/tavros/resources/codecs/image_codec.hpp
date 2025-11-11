#pragma once

#include <tavros/core/memory/dynamic_buffer.hpp>
#include <tavros/core/memory/buffer_view.hpp>

namespace tavros::resources
{

    /**
     * @brief Provides functionality for encoding and decoding image data.
     *
     * The `image_codec` class is responsible for converting between packed image formats
     * (e.g., PNG, JPEG) and raw pixel data stored in memory. It supports decoding from
     * compressed image data into a raw pixel buffer, and encoding from raw pixel data back
     * into a compressed format.
     *
     * @note Each call to `decode()` or `encode()` invalidates the data returned by any
     * previous call to either function, as the internal buffer is reused for performance.
     */
    class image_codec : core::noncopyable
    {
    public:
        /**
         * @brief Describes a view of raw image pixel data.
         *
         * This structure provides basic information about the decoded image, including
         * dimensions, channel count, row stride, and a memory view of the pixel data.
         */
        struct pixels_view
        {
            uint32 width = 0;                /// Image width in pixels.
            uint32 height = 0;               /// Image height in pixels.
            uint32 channels = 0;             /// Number of color channels (e.g., 3 for RGB, 4 for RGBA).
            uint32 stride = 0;               /// Number of bytes per row (including padding).

            core::buffer_view<uint8> pixels; /// View of the raw pixel data.
        };

        /**
         * @brief Specifies supported image encoding formats.
         *
         * Defines the available target formats for image encoding and decoding.
         * These correspond to common file types used for storing raster graphics.
         */
        enum class image_format
        {
            png, /// Portable Network Graphics format — lossless compression, supports alpha channel
            jpg, /// JPEG format — lossy compression, compact size, no alpha channel
            tga, /// Truevision TGA format — simple, can be uncompressed or RLE, optional alpha
            bmp, /// Windows Bitmap format — uncompressed or RLE, no built-in alpha
        };

    public:
        /**
         * @brief Constructs a new image codec.
         * @param alc Pointer to the memory allocator used for internal buffers.
         */
        explicit image_codec(tavros::core::allocator* alc);

        /**
         * @brief Destroys the image codec and releases any allocated resources.
         */
        ~image_codec();

        /**
         * @brief Decodes a packed image format into raw pixel data.
         *
         * This method reads encoded image data (e.g., PNG or JPEG) from the provided
         * buffer and outputs a raw pixel view in the desired number of channels.
         *
         * @param packed_pixels View of the compressed image data to decode.
         * @param required_channels Number of channels in the output image (default: 4).
         * @param y_flip If true, flips the image vertically during decoding.
         * @return A view of the decoded raw pixel data.
         *
         * @note The returned view becomes invalid after the next call to `decode()` or `encode()`.
         */
        pixels_view decode(core::buffer_view<uint8> packed_pixels, uint32 required_channels = 4, bool y_flip = false);

        /**
         * @brief Encodes raw pixel data into a compressed image format.
         *
         * Converts the provided raw pixel data into a packed format such as PNG or JPEG.
         *
         * @param pixels Reference to a view describing the input pixel data.
         * @param fmt Specifies the output image format (default: PNG).
         * @param y_flip If true, flips the image vertically before encoding.
         * @return A view of the encoded image data.
         *
         * @note The returned view becomes invalid after the next call to `encode()` or `decode()`.
         */
        core::buffer_view<uint8> encode(const pixels_view& pixels, image_format fmt = image_format::png, bool y_flip = false);

    private:
        core::dynamic_buffer<uint8> m_buffer;
    };

} // namespace tavros::resources

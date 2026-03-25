#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/unreachable.hpp>

namespace tavros::assets
{

    class image_view;

    /**
     * @brief Simple 2D image container for raw pixel data.
     *
     * Owns its pixel memory via dynamic_buffer (uninitialized, trivial types only).
     * Supports arbitrary pixel formats via component count.
     * Stride is in bytes and may be larger than width * components (e.g. aligned rows).
     */
    class image
    {
    public:
        /** @brief Supported pixel formats. */
        enum class pixel_format : uint32
        {
            none = 0,
            r8,
            rg8,
            rgb8,
            rgba8,
        };

        /** @brief Returns the number of components for a given pixel format. */
        [[nodiscard]] constexpr static uint32 component_count(pixel_format format) noexcept
        {
            switch (format) {
            case pixel_format::none:
                return 0;
            case pixel_format::r8:
                return 1;
            case pixel_format::rg8:
                return 2;
            case pixel_format::rgb8:
                return 3;
            case pixel_format::rgba8:
                return 4;
            default:
                TAV_UNREACHABLE();
            }
        }

    public:
        /**
         * @brief Decodes an image from a compressed byte buffer.
         *
         * Supports common formats (PNG, JPEG, etc.) via the underlying decoder.
         * The input data must remain valid for the duration of the call.
         *
         * @param data            View over the compressed image data.
         * @param required_format Required pixel format, none - use source channel count.
         * @param y_flip          If true, flips the image vertically after decoding (e.g. for OpenGL UV conventions).
         *
         * @return Decoded image. Check valid() on the result if decoding may fail.
         */
        [[nodiscard]] static image decode(core::buffer_view<uint8> data, pixel_format required_format = pixel_format::none, bool y_flip = false);

        /**
         * @brief Encodes an image into a compressed byte buffer.
         *
         * The output format is determined by the implementation (e.g. PNG).
         *
         * @param im     Image to encode. Must be valid().
         * @param y_flip If true, flips the image vertically before encoding.
         *
         * @return Buffer containing the encoded image data.
         */
        [[nodiscard]] static core::dynamic_buffer<uint8> encode(image_view im, bool y_flip = false);

    public:
        /** @brief Constructs an empty invalid image. */
        constexpr image() noexcept = default;

        /** @brief Destroys the image and releases any allocated resources. */
        ~image() noexcept = default;

        /**
         * @brief Constructs an image from an existing pixel buffer.
         *
         * Takes ownership of the provided buffer. The buffer must contain at least
         * stride * height bytes of pixel data.
         *
         * @param pixels Pixel data buffer (ownership transferred).
         * @param width  Width in pixels. Must be greater than zero.
         * @param height Height in pixels. Must be greater than zero.
         * @param format Pixel format. Must not be pixel_format::none.
         * @param stride Row stride in bytes. If 0, defaults to width * component_count(format).
         */
        explicit image(core::dynamic_buffer<uint8> pixels, uint32 width, uint32 height, pixel_format format, uint32 stride = 0)
            : m_width(width)
            , m_height(height)
            , m_format(format)
            , m_components(component_count(format))
            , m_stride(stride ? stride : width * component_count(format))
            , m_data(std::move(pixels))
        {
            TAV_ASSERT(m_width > 0 && m_height > 0 && m_format != pixel_format::none);
            TAV_ASSERT(m_data.size_bytes() >= static_cast<size_t>(m_stride) * static_cast<size_t>(m_height));
        }

        /**
         * @brief Allocates an uninitialized image of the given dimensions.
         *
         * Memory is allocated but not zero-initialized.
         *
         * @param width  Width in pixels. Must be greater than zero.
         * @param height Height in pixels. Must be greater than zero.
         * @param format Pixel format. Must not be pixel_format::none.
         * @param stride Row stride in bytes. If 0, defaults to width * component_count(format).
         */
        explicit image(uint32 width, uint32 height, pixel_format format, uint32 stride = 0)
            : m_width(width)
            , m_height(height)
            , m_format(format)
            , m_components(component_count(format))
            , m_stride(stride ? stride : width * component_count(format))
        {
            TAV_ASSERT(m_width > 0 && m_height > 0 && m_format != pixel_format::none);
            m_data.resize(static_cast<size_t>(m_stride) * static_cast<size_t>(m_height));
        }

        constexpr image(image&&) noexcept = default;
        constexpr image& operator=(image&&) noexcept = default;

        constexpr image(const image&) = default;
        constexpr image& operator=(const image&) = default;

        /**
         * @brief Returns a resized copy of the image.
         *
         * Resamples the current image to the specified dimensions and returns
         * the result as a new image instance. The original image remains unchanged.
         *
         * @param width  Target width in pixels.
         * @param height Target height in pixels.
         * @param srgb   If true, performs resizing in linear color space by converting
         *               from sRGB before filtering and converting back after. This
         *               produces more correct visual results for color textures.
         *
         * @return A new image containing the resized result.
         *
         * @note For non-color data (e.g., normal maps), `srgb` should typically be false.
         * @note Resizing may involve filtering which can slightly blur the image depending
         *       on the algorithm used.
         */
        [[nodiscard]] image resize(uint32 width, uint32 height, bool srgb = false) const;

        /** @brief Returns true if the image has allocated pixel data. */
        [[nodiscard]] bool valid() const noexcept
        {
            return !m_data.empty() && m_width > 0 && m_height > 0 && m_format != pixel_format::none;
        }

        /** @brief Returns the width of the image in pixels. */
        [[nodiscard]] uint32 width() const noexcept
        {
            return m_width;
        }

        /** @brief Returns the height of the image in pixels. */
        [[nodiscard]] uint32 height() const noexcept
        {
            return m_height;
        }

        /** @brief Returns the pixel format of the image. */
        [[nodiscard]] pixel_format format() const noexcept
        {
            return m_format;
        }

        /** @brief Returns the number of color channels (e.g. 3 for RGB, 4 for RGBA). */
        [[nodiscard]] uint32 components() const noexcept
        {
            return m_components;
        }

        /** @brief Returns the row stride in bytes. May be larger than width * components if rows are aligned. */
        [[nodiscard]] uint32 stride() const noexcept
        {
            return m_stride;
        }

        /** @brief Returns the total size of pixel data in bytes (stride * height). */
        [[nodiscard]] size_t size_bytes() const noexcept
        {
            return static_cast<size_t>(m_stride) * m_height;
        }

        /** @brief Returns the size of a single row of pixel data in bytes (width * components). */
        [[nodiscard]] size_t row_size_bytes() const noexcept
        {
            return static_cast<size_t>(m_width) * static_cast<size_t>(m_components);
        }

        /** @brief Returns a pointer to the start of pixel data. */
        [[nodiscard]] uint8* data() noexcept
        {
            return m_data.data();
        }

        /** @brief Returns a const pointer to the start of pixel data. */
        [[nodiscard]] const uint8* data() const noexcept
        {
            return m_data.data();
        }

        /**
         * @brief Returns a pointer to the start of row @p y.
         * @note Behavior is undefined if y >= height().
         */
        [[nodiscard]] uint8* row(uint32 y) noexcept
        {
            TAV_ASSERT(y < m_height);
            return m_data.data() + y * m_stride;
        }

        /**
         * @brief Returns a const pointer to the start of row @p y.
         * @note Behavior is undefined if y >= height().
         */
        [[nodiscard]] const uint8* row(uint32 y) const noexcept
        {
            TAV_ASSERT(y < m_height);
            return m_data.data() + y * m_stride;
        }

        /**
         * @brief Returns a pointer to the pixel at (x, y).
         * @note Behavior is undefined if x >= width() or y >= height().
         */
        [[nodiscard]] uint8* pixel(uint32 x, uint32 y) noexcept
        {
            TAV_ASSERT(x < m_width);
            return row(y) + static_cast<size_t>(x) * m_components;
        }

        /**
         * @brief Returns a const pointer to the pixel at (x, y).
         * @note Behavior is undefined if x >= width() or y >= height().
         */
        [[nodiscard]] const uint8* pixel(uint32 x, uint32 y) const noexcept
        {
            TAV_ASSERT(x < m_width);
            return row(y) + static_cast<size_t>(x) * m_components;
        }

    private:
        uint32       m_width = 0;
        uint32       m_height = 0;
        pixel_format m_format = pixel_format::none;
        uint32       m_components = 0;
        uint32       m_stride = 0;

        core::dynamic_buffer<uint8> m_data;
    };

} // namespace tavros::assets

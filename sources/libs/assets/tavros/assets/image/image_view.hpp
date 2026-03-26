#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/assets/image/image.hpp>

namespace tavros::assets
{

    /**
     * @brief Non-owning view over a rectangular region of an image.
     *
     * References a sub-region of an existing image without copying pixel data.
     * The source image must outlive the view.
     *
     * Row iteration must use stride() to advance between rows, as the view
     * may reference a sub-rectangle of a larger image.
     */
    class image_view
    {
    public:
        /**
         * @brief Constructs an empty invalid view.
         */
        constexpr image_view() noexcept = default;

        /**
         * @brief Constructs a view over raw pixel data.
         *
         * Allows wrapping an arbitrary memory buffer as an image view,
         * without requiring an existing image object.
         * The data must remain valid for the lifetime of the view.
         *
         * @param data   View over the raw pixel data. Must contain at least stride * height bytes.
         * @param width  Width in pixels. Must be greater than zero.
         * @param height Height in pixels. Must be greater than zero.
         * @param format Pixel format. Must not be pixel_format::none.
         * @param stride Row stride in bytes. If 0, defaults to width * component_count(format).
         */
        constexpr image_view(core::buffer_view<uint8> data, uint32 width, uint32 height, image::pixel_format format, uint32 stride = 0) noexcept
            : m_width(width)
            , m_height(height)
            , m_format(format)
            , m_components(image::component_count(format))
            , m_stride(stride ? stride : width * image::component_count(format))
            , m_data(data)
        {
            TAV_ASSERT(m_width > 0 && m_height > 0);
            TAV_ASSERT(m_format != image::pixel_format::none);
            TAV_ASSERT(m_data.size() >= static_cast<size_t>(m_stride) * m_height);
        }

        /**
         * @brief Constructs a view over the entire image.
         *
         * @param img Source image. Must outlive this view.
         */
        image_view(const image& im) noexcept
            : m_width(im.width())
            , m_height(im.height())
            , m_format(im.format())
            , m_components(im.components())
            , m_stride(im.stride())
            , m_data({im.data(), im.size_bytes()})
        {
        }

        /**
         * @brief Constructs a view over a sub-rectangle of an image.
         *
         * @param img    Source image. Must outlive this view.
         * @param x      X offset in pixels. Must be less than img.width().
         * @param y      Y offset in pixels. Must be less than img.height().
         * @param width  Width of the region in pixels. Must be greater than zero.
         * @param height Height of the region in pixels. Must be greater than zero.
         *
         * @note The region [x, x + width) x [y, y + height) must be fully contained within the source image.
         */
        image_view(const image& im, uint32 x, uint32 y, uint32 width, uint32 height) noexcept
            : m_width(width)
            , m_height(height)
            , m_format(im.format())
            , m_components(im.components())
            , m_stride(im.stride())
            , m_data({im.data() + y * im.stride() + x * im.components(), im.size_bytes() - y * im.stride() + x * im.components()})
        {
            TAV_ASSERT(x + width <= im.width());
            TAV_ASSERT(y + height <= im.height());
            TAV_ASSERT(width > 0 && height > 0);
        }

        /**
         * @brief Returns a resized copy of the image.
         * @see image::resize
         */
        [[nodiscard]] image resize(uint32 width, uint32 height, bool srgb = false)
        {
            return image::resize(*this, width, height, srgb);
        }

        /** @brief Returns true if the view references valid pixel data. */
        [[nodiscard]] constexpr bool valid() const noexcept
        {
            return m_data.data() != nullptr && m_width > 0 && m_height > 0 && m_format != image::pixel_format::none;
        }

        /** @brief Returns the width of the view in pixels. */
        [[nodiscard]] constexpr uint32 width() const noexcept
        {
            return m_width;
        }

        /** @brief Returns the height of the view in pixels. */
        [[nodiscard]] constexpr uint32 height() const noexcept
        {
            return m_height;
        }

        /** @brief Returns the pixel format. */
        [[nodiscard]] constexpr image::pixel_format format() const noexcept
        {
            return m_format;
        }

        /** @brief Returns the number of color channels. */
        [[nodiscard]] constexpr uint32 components() const noexcept
        {
            return m_components;
        }

        /**
         * @brief Returns the row stride in bytes of the source image.
         *
         * Use this to advance between rows - do NOT use width() * components().
         */
        [[nodiscard]] constexpr uint32 stride() const noexcept
        {
            return m_stride;
        }

        /** @brief Returns the total size of pixel data in bytes (stride * height). */
        [[nodiscard]] size_t size_bytes() const noexcept
        {
            return static_cast<size_t>(m_stride) * m_height;
        }

        /** @brief Returns the size of a single row of the view in bytes (width * components). */
        [[nodiscard]] constexpr size_t row_size_bytes() const noexcept
        {
            return static_cast<size_t>(m_width) * m_components;
        }

        /** @brief Returns a const pointer to the first pixel of the view. */
        [[nodiscard]] constexpr const uint8* data() const noexcept
        {
            return m_data.data();
        }

        /**
         * @brief Returns a const pointer to the start of row @p y within the view.
         * @note Behavior is undefined if y >= height().
         */
        [[nodiscard]] const uint8* row(uint32 y) const noexcept
        {
            TAV_ASSERT(y < m_height);
            return m_data.data() + y * m_stride;
        }

        /**
         * @brief Returns a const pointer to the pixel at (x, y) within the view.
         * @note Behavior is undefined if x >= width() or y >= height().
         */
        [[nodiscard]] const uint8* pixel(uint32 x, uint32 y) const noexcept
        {
            TAV_ASSERT(x < m_width);
            return row(y) + static_cast<size_t>(x) * m_components;
        }

    private:
        uint32              m_width = 0;
        uint32              m_height = 0;
        image::pixel_format m_format = image::pixel_format::none;
        uint32              m_components = 0;
        uint32              m_stride = 0; // stride of the source image, not the view

        core::buffer_view<uint8> m_data = nullptr;
    };

} // namespace tavros::assets

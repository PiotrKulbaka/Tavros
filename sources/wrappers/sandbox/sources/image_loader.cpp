#include "image_loader.hpp"

#include <tavros/core/logger/logger.hpp>
#include <stb/stb_image.h>

namespace app
{

    image_loader::image_loader(tavros::core::allocator* alc)
        : m_buffer(alc)
    {
    }

    image_loader::image_info image_loader::load_image_info(tavros::core::string_view filename) const
    {
        int x = 0, y = 0, channels_in_file = 0;
        if (!stbi_info(filename.data(), &x, &y, &channels_in_file)) {
            tavros::core::logger::print(
                tavros::core::severity_level::error,
                "image_loader",
                "Failed to get image info `{}`",
                filename
            );
            return {};
        }

        image_info info{};
        info.width = static_cast<uint32>(x);
        info.height = static_cast<uint32>(y);
        info.channels = static_cast<uint32>(channels_in_file);
        return info;
    }

    image_loader::pixels_view image_loader::load_image(tavros::core::string_view filename, uint32 required_channels)
    {
        static uint8_t white_pixel[4] = {255, 255, 255, 255};

        auto info = load_image_info(filename);
        if (info.width == 0 || info.height == 0) {
            tavros::core::logger::print(
                tavros::core::severity_level::error,
                "image_loader",
                "Failed to load image '{}'. Returning fallback 1x1 white pixel",
                filename
            );
            return {1, 1, required_channels, required_channels, white_pixel};
        }

        int    desired_channels = static_cast<int>(required_channels);
        size_t required_size = static_cast<size_t>(info.width) * static_cast<size_t>(info.height) * static_cast<size_t>(required_channels);
        m_buffer.ensure_size(required_size);

        int    x = 0, y = 0, channels_in_file = 0;
        uint8* pixels = stbi_load(filename.data(), &x, &y, &channels_in_file, desired_channels);

        TAV_ASSERT(x == info.width);
        TAV_ASSERT(y == info.height);

        if (!pixels) {
            tavros::core::logger::print(
                tavros::core::severity_level::error,
                "image_loader",
                "Failed to load image '{}'. Returning fallback 1x1 white pixel",
                filename
            );
            return {1, 1, required_channels, required_channels, white_pixel};
        }

        std::memcpy(m_buffer.data(), pixels, required_size);
        stbi_image_free(pixels);

        uint32 stride = static_cast<uint32>(info.width * required_channels);
        return {info.width, info.height, required_channels, stride, m_buffer.data()};
    }

} // namespace app

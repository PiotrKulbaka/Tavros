#include <tavros/assets/image/image_codec.hpp>

#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/logger/logger.hpp>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace
{
    tavros::core::logger logger("image_codec");
}

namespace tavros::assets
{

    core::dynamic_buffer<uint8> image_codec::decode(core::buffer_view<uint8> data, uint32& w, uint32& h, uint32& c, uint32& stride, uint32 rc, bool y_flip)
    {
        TAV_ASSERT(rc == 0 || rc == 1 || rc == 3 || rc == 4);

        w = h = c = stride = 0;

        if (data.empty()) {
            ::logger.error("Failed to decode pixels: data is empty");
            return {};
        }

        int info_x = 0, info_y = 0, info_channels_in_file = 0;
        if (!stbi_info_from_memory(data.data(), static_cast<int>(data.size()), &info_x, &info_y, &info_channels_in_file)) {
            ::logger.error("Failed to decode pixels: stbi_info_from_memory is failed");
            return {};
        }

        stbi_set_flip_vertically_on_load_thread(y_flip);

        int32 required_channels = rc == 0 ? info_channels_in_file : static_cast<int32>(rc);

        int    x = 0, y = 0, channels_in_file = 0;
        uint8* pixels = stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &x, &y, &channels_in_file, required_channels);
        if (!pixels) {
            ::logger.error("Failed to decode pixels: stbi_load_from_memory is failed");
            return {};
        }

        TAV_ASSERT(x == info_x);
        TAV_ASSERT(y == info_y);
        TAV_ASSERT(channels_in_file == info_channels_in_file);

        w = static_cast<uint32>(x);
        h = static_cast<uint32>(y);
        c = static_cast<uint32>(channels_in_file);
        stride = c * w;

        const auto                  required_size = static_cast<size_t>(stride) * static_cast<size_t>(h);
        core::dynamic_buffer<uint8> buffer(pixels, required_size);

        stbi_image_free(pixels);

        return buffer;
    }

    core::dynamic_buffer<uint8> image_codec::encode(core::buffer_view<uint8> pixels, uint32 w, uint32 h, uint32 c, uint32 stride, image_format fmt, bool y_flip)
    {
        struct ctx_t
        {
            tavros::core::dynamic_buffer<uint8>* buf = nullptr;
            size_t                               size = 0;
        };

        core::dynamic_buffer<uint8> buffer;
        ctx_t                       ctx{&buffer, 0};

        auto write_fn = [](void* ctx_ptr, void* data, int size) {
            auto* ctx = reinterpret_cast<ctx_t*>(ctx_ptr);
            auto  sz = static_cast<size_t>(size);
            ctx->buf->reserve(sz);
            ctx->buf->copy_from(static_cast<uint8*>(data), sz);
            ctx->size = sz;
        };

        stbi_flip_vertically_on_write(y_flip);

        auto  wi = static_cast<int32>(w);
        auto  hi = static_cast<int32>(h);
        auto  ci = static_cast<int32>(c);
        auto* p = reinterpret_cast<const unsigned char*>(pixels.data());
        auto  si = static_cast<int32>(stride);

        int32 success = 0;

        switch (fmt) {
        case image_format::png:
            success = stbi_write_png_to_func(write_fn, &ctx, wi, hi, ci, p, si);
            break;

        case image_format::jpg:
            success = stbi_write_jpg_to_func(write_fn, &ctx, wi, hi, ci, p, si);
            break;

        case image_format::tga:
            if (si != wi * ci) {
                ::logger.warning("Encode pixels: pixels are not tightly packed");
            }
            success = stbi_write_tga_to_func(write_fn, &ctx, wi, hi, ci, p);
            break;

        case image_format::bmp:
            if (si != wi * ci) {
                ::logger.warning("Encode pixels: pixels are not tightly packed");
            }
            success = stbi_write_bmp_to_func(write_fn, &ctx, wi, hi, ci, p);
            break;
        default:
            TAV_UNREACHABLE();
        }

        if (!success) {
            ::logger.error("Failed to encode pixels: stbi_write_png_to_func is failed");
            return {};
        }

        return buffer;
    }

} // namespace tavros::assets

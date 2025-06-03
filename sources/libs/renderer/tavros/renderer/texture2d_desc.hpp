#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    enum class texture_filter : uint8
    {
        nearest,
        linear,
    };

    enum class texture_wrap : uint8
    {
        repeat,
        clamp_to_edge, 
    };

    enum class pixel_format : uint8
    {
        rgba,
        rgb,
    };

    struct texture2d_desc
    {
        texture_desc() = default;

        texture2d_desc& set_filter(texture_filter filter)
        {
            min_filter = filter;
            mag_filter = filter;
            return *this;
        }

        texture2d_desc& set_wrap(texture_wrap wrap)
        { 
            wrap_s = wrap;
            wrap_t = wrap;
            return *this;
        }

        texture2d_desc& set_format(pixel_format format)
        {
            this->format = format;
            return *this;
        }

        texture2d_desc& set_gen_mipmaps(bool gen_mipmaps)
        {
            this->gen_mipmaps = gen_mipmaps;
            return *this;
        }

        texture2d_desc& set_size(int32 width, int32 height)
        {
            this->width = width;
            this->height = height;
            return *this;
        }
        
        texture_filter min_filter = texture_filter::nearest;
        texture_filter mag_filter = texture_filter::nearest;
        texture_wrap wrap_s = texture_wrap::repeat;
        texture_wrap wrap_t = texture_wrap::repeat;
        pixel_format format = pixel_format::rgba;
        bool gen_mipmaps = false;
        int32 width = 0;
        int32 height = 0;
    };

} // namespace tavros::renderer

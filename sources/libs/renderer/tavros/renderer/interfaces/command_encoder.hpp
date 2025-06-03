#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer::interfaces
{

    class command_encoder
    {
    public:
        virtual ~command_encoder() = default;

        virtual void bind_texture(texture_handle texture, uint32 slot) = 0;

        virtual void bind_buffer(buffer_handle buffer) = 0;

        virtual void draw(const drawable_info&) = 0;
    };

} // namespace tavros::renderer::interfaces

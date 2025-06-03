#pragma once

#include <tavros/renderer/interfaces/texture2d.hpp>

#include <tavros/renderer/texture2d_desc.hpp>


namespace tavros::renderer::interfaces
{

    class graphics_device
    {
    public:
        virtual ~graphics_device() = default;

        virtual texture_handle create_texture(const texture2d_desc&, int32 stride = 0, const uint8* pixels = nullptr) = 0;

        virtual void update_texture(texture_handle, const texture2d_desc&, void* pixels) = 0;

        virtual void destroy_texture(texture_handle) = 0;
    };

} // namespace tavros::renderer

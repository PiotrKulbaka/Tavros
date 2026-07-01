#pragma once

#include <tavros/core/resource/resource_registry.hpp>
#include <tavros/renderer/texture/texture_desc.hpp>
#include <tavros/renderer/upload_context.hpp>
#include <tavros/assets/image/image_view.hpp>

namespace tavros::renderer
{

    class texture : core::noncopyable
    {
    public:
        texture(rhi::graphics_device* gdevice, upload_context& upctx, assets::image_view im, const texture_desc& desc, bool y_flip);
        texture(texture&&) noexcept;
        ~texture() noexcept;

        rhi::texture_type type() const noexcept
        {
            return m_type;
        }

        rhi::pixel_format format() const noexcept
        {
            return m_format;
        }

        uint32 width() const noexcept
        {
            return m_width;
        }

        uint32 height() const noexcept
        {
            return m_height;
        }

        uint32 depth() const noexcept
        {
            return m_depth;
        }

        uint32 array_layers() const noexcept
        {
            return m_array_layers;
        }

        rhi::texture_handle gpu_texture() const noexcept
        {
            return m_texture;
        }

    private:
        rhi::graphics_device* m_gdevice;

        rhi::texture_handle m_texture;
        rhi::texture_type   m_type;
        rhi::pixel_format   m_format;

        uint32 m_width;
        uint32 m_height;
        uint32 m_depth;
        uint32 m_array_layers;
    };

    using texture_ref = core::resource_ref<texture>;

} // namespace tavros::renderer

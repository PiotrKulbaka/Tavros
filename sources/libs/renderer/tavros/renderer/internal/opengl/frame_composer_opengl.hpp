#pragma once

#include <tavros/renderer/rhi/frame_composer.hpp>

#include <tavros/renderer/internal/opengl/context_opengl.hpp>

#include <tavros/renderer/internal/opengl/command_queue_opengl.hpp>

namespace tavros::renderer::rhi
{
    class graphics_device_opengl;


    class frame_composer_opengl : public frame_composer
    {
    public:
        static core::unique_ptr<frame_composer> create(graphics_device_opengl* device, const frame_composer_create_info& info, void* native_handle);

    public:
        frame_composer_opengl(graphics_device_opengl* device, core::unique_ptr<context_opengl> context, const frame_composer_create_info& info);

        ~frame_composer_opengl() override;

        void resize(uint32 width, uint32 height) override;

        virtual uint32 width() const noexcept override;

        virtual uint32 height() const noexcept override;

        virtual framebuffer_handle backbuffer() const noexcept override;

        virtual void present() override;

    private:
        graphics_device_opengl*          m_device;
        core::unique_ptr<context_opengl> m_context;
        frame_composer_create_info       m_info;
        framebuffer_handle               m_backbuffer;
    };

} // namespace tavros::renderer::rhi

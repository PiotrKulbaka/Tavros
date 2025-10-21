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

        virtual void begin_frame() override;

        virtual void end_frame() override;

        virtual command_queue* create_command_queue() override;

        virtual void submit_command_queue(command_queue* queue) override;

        virtual bool is_frame_complete() override;

        virtual void wait_for_frame_complete() override;

    private:
        graphics_device_opengl*          m_device;
        core::unique_ptr<context_opengl> m_context;
        frame_composer_create_info       m_info;
        framebuffer_handle               m_backbuffer;
        fence_handle                     m_fence;

        bool m_frame_started = false;

        core::unique_ptr<command_queue_opengl> m_internal_command_queue; // Temporary object, will be deleted soon
    };

} // namespace tavros::renderer::rhi

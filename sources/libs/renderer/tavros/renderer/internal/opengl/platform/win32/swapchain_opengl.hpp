#pragma once

#include <tavros/renderer/internal/opengl/graphics_device_opengl.hpp>

#include <Windows.h>

namespace tavros::renderer
{

    class swapchain_opengl final : public swapchain
    {
    public:
        swapchain_opengl(graphics_device_opengl* device, const swapchain_desc& desc, HWND hWnd, HDC hDC, HGLRC hGLRC);

        ~swapchain_opengl() override;

        uint32 acquire_next_backbuffer_index() noexcept override;

        framebuffer_handle get_framebuffer(uint32 backbuffer_index) override;

        void present(uint32 buffer_index) override;

        void resize(uint32 width, uint32 height) override;

        uint32 width() const noexcept override;

        uint32 height() const noexcept override;

        void make_current();

        void make_inactive();

    private:
        graphics_device_opengl* m_device = nullptr;
        swapchain_desc          m_desc;
        HWND                    m_hWnd = nullptr;  // Handle to the window
        HDC                     m_hDC = nullptr;   // Handle to the device context
        HGLRC                   m_hGLRC = nullptr; // Handle to the OpenGL context
        uint32                  m_width = 0;
        uint32                  m_height = 0;
        framebuffer_handle      m_framebuffer;
    };

} // namespace tavros::renderer

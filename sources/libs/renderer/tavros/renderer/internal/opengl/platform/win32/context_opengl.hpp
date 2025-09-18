#pragma once

#include <tavros/renderer/internal/opengl/context_opengl.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/nonmovable.hpp>

#include <Windows.h>

namespace tavros::renderer::rhi
{

    class context_opengl_win32 : public context_opengl, core::noncopyable, core::nonmovable
    {
    public:
        context_opengl_win32(HWND hWnd, HDC hDC, HGLRC hGLRC);

        ~context_opengl_win32() override;

        void make_current() override;

        void make_inactive() override;

        void swap_buffers() override;

        bool is_current() override;

    private:
        HWND  m_hWnd = nullptr;  // Handle to the window
        HDC   m_hDC = nullptr;   // Handle to the device context
        HGLRC m_hGLRC = nullptr; // Handle to the OpenGL context
    };

} // namespace tavros::renderer::rhi

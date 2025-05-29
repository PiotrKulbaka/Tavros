#pragma once

#include <tavros/renderer/interfaces/gl_context.hpp>

#include <Windows.h>

namespace tavros::renderer
{
    class gl_context : public interfaces::gl_context, core::noncopyable, core::nonmovable
    {
    public:
        gl_context(HWND hWnd);

        virtual ~gl_context() override;

        virtual void make_current() override;

        virtual void make_inactive() override;

        virtual void swap_buffers() override;

        virtual bool is_current() override;

    private:
        HWND  m_hWnd = nullptr;  // Handle to the window
        HDC   m_hDC = nullptr;   // Handle to the device context
        HGLRC m_hGLRC = nullptr; // Handle to the OpenGL context
    };
} // namespace tavros::renderer

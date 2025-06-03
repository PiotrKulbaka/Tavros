#pragma once

#include <tavros/core/types.hpp>

#include <Windows.h>

class opengl_swapchain
{
public:
    opengl_swapchain(handle hWnd);
    ~opengl_swapchain();

    bool init();
    void present();
    void activate();         // Activate OpenGL context
    void deactivate();       // Deactivate OpenGL context
private:
    HWND  m_hWnd = nullptr;  // Handle to the window
    HDC   m_hDC = nullptr;   // Handle to the device context
    HGLRC m_hGLRC = nullptr; // Handle to the OpenGL context
};

#include "opengl_swapchain.hpp"

#include <tavros/system/platform/win32/utils.hpp>
#include <tavros/system/library.hpp>
#include <tavros/core/logger/logger.hpp>
#include <WinUser.h>
#include <windef.h>
#include <wingdi.h>
#include <cassert>

namespace
{
    tavros::core::logger logger("opengl_swapchain");

    typedef HGLRC(WINAPI* fn_wglCreateContext_t)(HDC);
    fn_wglCreateContext_t _wglCreateContext;
    typedef BOOL(WINAPI* fn_wglDeleteContext_t)(HGLRC);
    fn_wglDeleteContext_t _wglDeleteContext;
    typedef BOOL(WINAPI* fn_wglMakeCurrent_t)(HDC, HGLRC);
    fn_wglMakeCurrent_t _wglMakeCurrent;

    static tavros::system::library s_opengl_lib; // Instatnce to the OpenGL library.

    bool load_wgl_funcs()
    {
        if (s_opengl_lib.is_open()) {
            return true;
        }

        if (!s_opengl_lib.open("opengl32")) {
            return false;
        }
        logger.info("Library OpenGL loaded successfully.");

        _wglCreateContext = static_cast<fn_wglCreateContext_t>(s_opengl_lib.get_symbol("wglCreateContext"));
        _wglDeleteContext = static_cast<fn_wglDeleteContext_t>(s_opengl_lib.get_symbol("wglDeleteContext"));
        _wglMakeCurrent = static_cast<fn_wglMakeCurrent_t>(s_opengl_lib.get_symbol("wglMakeCurrent"));

        if (_wglCreateContext == nullptr || _wglDeleteContext == nullptr || _wglMakeCurrent == nullptr) {
            logger.info("Error loading wgl functions.");
            s_opengl_lib.close();
            return false;
        }

        return true;
    }
} // namespace


opengl_swapchain::opengl_swapchain(handle hWnd)
{
    m_hWnd = static_cast<HWND>(hWnd);
    assert(hWnd != nullptr);
}

opengl_swapchain::~opengl_swapchain()
{
    if (m_hGLRC) {
        if (!_wglDeleteContext(m_hGLRC)) {
            ::logger.error("Detete OpenGL context failed: %s", tavros::system::last_win_error_str());
        }
        m_hGLRC = nullptr;
    }

    if (m_hDC) {
        if (!ReleaseDC(m_hWnd, m_hDC)) {
            ::logger.error("ReleaseDC failed.");
        }
        m_hDC = nullptr;
    }
    m_hWnd = nullptr;
}

bool opengl_swapchain::init()
{
    m_hDC = GetDC(m_hWnd);
    if (!m_hDC) {
        ::logger.error("GetDC failed.");
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int32 pixel_format = ChoosePixelFormat(m_hDC, &pfd);
    if (!pixel_format) {
        ::logger.error("ChoosePixelFormat: %s", tavros::system::last_win_error_str());
        return false;
    }
    ::logger.info("Pixel format selected [ColorBits: %d, DepthBits: %d, StencilBits: %d]", pfd.cColorBits, pfd.cDepthBits, pfd.cStencilBits);
    if (!SetPixelFormat(m_hDC, pixel_format, &pfd)) {
        ::logger.error("SetPixelFormat failed: %s", tavros::system::last_win_error_str());
        return false;
    }

    if (!load_wgl_funcs()) {
        return false;
    }

    m_hGLRC = _wglCreateContext(m_hDC);
    if (!m_hGLRC) {
        ::logger.error("Create OpenGL context failed: %s", tavros::system::last_win_error_str());
        return false;
    }

    ::logger.info("OpenGL swapchain has been initialized.", tavros::system::last_win_error_str());
    return true;
}

void opengl_swapchain::present()
{
    if (!SwapBuffers(m_hDC)) {
        ::logger.error("OpenGL swap buffers failed: %s", tavros::system::last_win_error_str());
    }
}

void opengl_swapchain::activate()
{
    if (!_wglMakeCurrent(m_hDC, m_hGLRC)) {
        ::logger.error("OpenGL activate context failed: %s", tavros::system::last_win_error_str());
    }
}

void opengl_swapchain::deactivate()
{
    if (!_wglMakeCurrent(nullptr, nullptr)) {
        ::logger.error("OpenGL deactivate context failed: %s", tavros::system::last_win_error_str());
    }
}

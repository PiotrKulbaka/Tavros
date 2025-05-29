#include <tavros/renderer/internal/platform/win32/gl_context.hpp>

#include <WinUser.h>
#include <windef.h>
#include <wingdi.h>

using namespace tavros::renderer;

namespace
{
    tavros::core::logger logger("gl_context");

    typedef HGLRC(WINAPI* fn_wglCreateContext_t)(HDC);
    fn_wglCreateContext_t _wglCreateContext;
    typedef BOOL(WINAPI* fn_wglDeleteContext_t)(HGLRC);
    fn_wglDeleteContext_t _wglDeleteContext;
    typedef BOOL(WINAPI* fn_wglMakeCurrent_t)(HDC, HGLRC);
    fn_wglMakeCurrent_t _wglMakeCurrent;
    typedef HGLRC(WINAPI* fn_wglGetCurrentContext)();
    fn_wglGetCurrentContext _wglGetCurrentContext;

    bool load_wgl_funcs()
    {
        auto gl_lib = LoadLibrary("opengl32.dll");
        if (gl_lib == NULL) {
            logger.error("Error loading OpenGL library.");
            return false;
        }

        _wglCreateContext = static_cast<fn_wglCreateContext_t>(static_cast<void*>(GetProcAddress(gl_lib, "wglCreateContext")));
        _wglDeleteContext = static_cast<fn_wglDeleteContext_t>(static_cast<void*>(GetProcAddress(gl_lib, "wglDeleteContext")));
        _wglMakeCurrent = static_cast<fn_wglMakeCurrent_t>(static_cast<void*>(GetProcAddress(gl_lib, "wglMakeCurrent")));
        _wglGetCurrentContext = static_cast<fn_wglGetCurrentContext>(static_cast<void*>(GetProcAddress(gl_lib, "wglGetCurrentContext")));

        FreeLibrary((HMODULE)gl_lib);

        if (_wglCreateContext == nullptr || _wglDeleteContext == nullptr || _wglMakeCurrent == nullptr || _wglGetCurrentContext == nullptr) {
            logger.info("Error loading wgl functions.");
            return false;
        }

        return true;
    }
} // namespace



gl_context_uptr interfaces::gl_context::create(handle h)
{
    return tavros::core::make_unique<tavros::renderer::gl_context>(static_cast<HWND>(h));
}

gl_context::gl_context(HWND hWnd)
{
    TAV_ASSERT(hWnd);
    m_hWnd = hWnd;

    m_hDC = GetDC(m_hWnd);
    if (!m_hDC) {
        ::logger.error("GetDC failed.");
        return;
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
        ::logger.error("ChoosePixelFormat"/*, last_win_error_str()*/);
        return;
    }
    ::logger.info("Pixel format selected [ColorBits: %d, DepthBits: %d, StencilBits: %d]", pfd.cColorBits, pfd.cDepthBits, pfd.cStencilBits);
    if (!SetPixelFormat(m_hDC, pixel_format, &pfd)) {
        ::logger.error("SetPixelFormat failed"/*, last_win_error_str()*/);
        return;
    }

    if (!load_wgl_funcs()) {
        return;
    }

    m_hGLRC = _wglCreateContext(m_hDC);
    if (!m_hGLRC) {
        ::logger.error("Create OpenGL context failed"/*, last_win_error_str()*/);
        return;
    }

    ::logger.info("OpenGL swapchain has been initialized.");
}

gl_context::~gl_context()
{
    if (m_hGLRC) {
        if (!_wglDeleteContext(m_hGLRC)) {
            ::logger.error("Detete OpenGL context failed"/*, last_win_error_str()*/); // TODO: print error
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

void gl_context::make_current()
{
    if (!_wglMakeCurrent(m_hDC, m_hGLRC)) {
        ::logger.error("OpenGL activate context failed"/*, last_win_error_str()*/);
    }
}

void gl_context::make_inactive()
{
    if (!_wglMakeCurrent(nullptr, nullptr)) {
        ::logger.error("OpenGL deactivate context failed"/*, last_win_error_str()*/);
    }
}

void gl_context::swap_buffers()
{
    if (!SwapBuffers(m_hDC)) {
        ::logger.error("OpenGL swap buffers failed"/*, last_win_error_str()*/);
    }
}

bool gl_context::is_current()
{
    return _wglGetCurrentContext() == m_hGLRC;
}

#include <tavros/renderer/internal/opengl/platform/win32/context_opengl.hpp>

#include <tavros/core/prelude.hpp>
#include <tavros/core/scoped_owner.hpp>

#include <WinUser.h>
#include <windef.h>
#include <wingdi.h>

#include <glad/glad.h>

using namespace tavros::renderer::rhi;

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

        FreeLibrary((HMODULE) gl_lib);

        if (_wglCreateContext == nullptr || _wglDeleteContext == nullptr || _wglMakeCurrent == nullptr || _wglGetCurrentContext == nullptr) {
            logger.info("Error loading wgl functions.");
            return false;
        }

        return true;
    }
} // namespace

namespace tavros::renderer::rhi
{

    core::unique_ptr<context_opengl> context_opengl::create(const frame_composer_info& info, void* native_handle)
    {
        // Validate the swapchain info
        // Validate width and height
        if (info.width == 0 || info.height == 0) {
            ::logger.error("Swapchain width and height must be greater than zero.");
            return nullptr;
        }

        // Validate buffer count
        if (info.buffer_count == 0) {
            ::logger.error("Swapchain buffer count must be greater than zero.");
            return nullptr;
        }

        if (info.buffer_count != 2 && info.buffer_count != 3) {
            ::logger.error("Swapchain buffer count must be 2 or 3.");
            return nullptr;
        }

        // Validate color attachment
        if (info.color_attachment_format != pixel_format::rgba8un) {
            ::logger.error("Swapchain color attachment format must be rgba8un.");
            return nullptr;
        }

        // Validate depth/stencil attachment
        if (info.depth_stencil_attachment_format != pixel_format::depth24_stencil8 && info.depth_stencil_attachment_format != pixel_format::none) {
            ::logger.error("Swapchain depth/stencil attachment format must be depth24stencil8 or none.");
            return nullptr;
        }

        TAV_ASSERT(native_handle);
        HWND hWnd = static_cast<HWND>(native_handle);

        HDC hDC = GetDC(hWnd);
        if (!hDC) {
            ::logger.error("GetDC failed to create swapchain.");
            return nullptr;
        }

        auto dc_owner = core::make_scoped_owner(hDC, [hWnd](HDC hDC) {
            ReleaseDC(hWnd, hDC);
        });

        PIXELFORMATDESCRIPTOR pfd;
        memset(&pfd, 0, sizeof(pfd));
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = info.depth_stencil_attachment_format == pixel_format::depth24_stencil8 ? 24 : 0;
        pfd.cStencilBits = info.depth_stencil_attachment_format == pixel_format::depth24_stencil8 ? 8 : 0;
        pfd.iLayerType = PFD_MAIN_PLANE;

        int32 pixel_format = ChoosePixelFormat(hDC, &pfd);
        if (!pixel_format) {
            ::logger.error("ChoosePixelFormat" /*, last_win_error_str()*/);
            return nullptr;
        }

        ::logger.info("Pixel format selected [ColorBits: %d, DepthBits: %d, StencilBits: %d]", pfd.cColorBits, pfd.cDepthBits, pfd.cStencilBits);
        if (!SetPixelFormat(hDC, pixel_format, &pfd)) {
            ::logger.error("SetPixelFormat failed" /*, last_win_error_str()*/);
            return nullptr;
        }

        if (!load_wgl_funcs()) {
            return nullptr;
        }

        HGLRC hGLRC = _wglCreateContext(hDC);
        if (!hGLRC) {
            ::logger.error("Create OpenGL context failed" /*, last_win_error_str()*/);
            return nullptr;
        }

        if (!_wglMakeCurrent(hDC, hGLRC)) {
            ::logger.error("Activate OpenGL context failed" /*, last_win_error_str()*/);
            return nullptr;
        }

        static bool glad_initialized = false;
        if (!glad_initialized) {
            glad_initialized = true;
            if (!gladLoadGL()) {
                ::logger.error("gladLoadGL failed" /*, last_win_error_str()*/);
                return nullptr;
            }
        }

        return core::make_unique<context_opengl_win32>(hWnd, dc_owner.release(), hGLRC);
    }

    context_opengl_win32::context_opengl_win32(HWND hWnd, HDC hDC, HGLRC hGLRC)
        : m_hWnd(hWnd)
        , m_hDC(hDC)
        , m_hGLRC(hGLRC)
    {
        TAV_ASSERT(hWnd);
        TAV_ASSERT(hDC);
        TAV_ASSERT(hGLRC);
    }

    context_opengl_win32::~context_opengl_win32()
    {
        make_inactive();
        if (m_hGLRC) {
            if (!_wglDeleteContext(m_hGLRC)) {
                ::logger.error("Detete OpenGL context failed" /*, last_win_error_str()*/); // TODO: print error
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

    void context_opengl_win32::make_current()
    {
        if (!_wglMakeCurrent(m_hDC, m_hGLRC)) {
            ::logger.error("OpenGL activate context failed" /*, last_win_error_str()*/);
        }
    }

    void context_opengl_win32::make_inactive()
    {
        if (!_wglMakeCurrent(nullptr, nullptr)) {
            ::logger.error("OpenGL deactivate context failed" /*, last_win_error_str()*/);
        }
    }

    void context_opengl_win32::swap_buffers()
    {
        if (!SwapBuffers(m_hDC)) {
            ::logger.error("OpenGL swap buffers failed" /*, last_win_error_str()*/);
        }
    }

    bool context_opengl_win32::is_current()
    {
        return _wglGetCurrentContext() == m_hGLRC;
    }

} // namespace tavros::renderer::rhi

#include <tavros/renderer/internal/opengl/platform/win32/context_opengl.hpp>

#include <tavros/renderer/internal/opengl/type_conversions.hpp>
#include <tavros/renderer/rhi/string_utils.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/raii/scoped_owner.hpp>

#include <mutex>

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
            logger.error("Error loading wgl functions.");
            return false;
        }

        return true;
    }
} // namespace

namespace tavros::renderer::rhi
{

    core::unique_ptr<context_opengl> context_opengl::create(const frame_composer_create_info& info, void* native_handle)
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
        auto color_info = to_gl_wnd_fb_info(info.color_attachment_format);
        if (!(color_info.supported && color_info.color_bits > 0)) {
            ::logger.error("Swapchain color format {} is not supported", info.color_attachment_format);
            return nullptr;
        }

        // Validate depth/stencil attachment
        auto depth_stencil_info = to_gl_wnd_fb_info(info.depth_stencil_attachment_format);
        auto is_supported_depth_stencil = depth_stencil_info.supported && (depth_stencil_info.depth_bits > 0 || depth_stencil_info.stencil_bits > 0);
        if (!(is_supported_depth_stencil || info.depth_stencil_attachment_format == rhi::pixel_format::none)) {
            ::logger.error("Swapchain depth/stencil format {} is not supported", info.depth_stencil_attachment_format);
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
        pfd.cColorBits = static_cast<BYTE>(color_info.color_bits);
        pfd.cDepthBits = static_cast<BYTE>(depth_stencil_info.depth_bits);
        pfd.cStencilBits = static_cast<BYTE>(depth_stencil_info.stencil_bits);
        pfd.iLayerType = PFD_MAIN_PLANE;

        int32 pixel_format = ChoosePixelFormat(hDC, &pfd);
        if (!pixel_format) {
            ::logger.error("ChoosePixelFormat" /*, last_win_error_str()*/);
            return nullptr;
        }

        if (!SetPixelFormat(hDC, pixel_format, &pfd)) {
            ::logger.error("SetPixelFormat failed" /*, last_win_error_str()*/);
            return nullptr;
        }

        ::logger.info(
            "Pixel format selected [ColorBits: {}, DepthBits: {}, StencilBits: {}]",
            fmt::styled_param(pfd.cColorBits),
            fmt::styled_param(pfd.cColorBits),
            fmt::styled_param(pfd.cDepthBits),
            fmt::styled_param(pfd.cStencilBits)
        );

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

        {
            static bool                 glad_initialized = false;
            static std::mutex           mtx;
            std::lock_guard<std::mutex> lock(mtx);
            if (!glad_initialized) {
                if (!gladLoadGL()) {
                    ::logger.error("gladLoadGL failed" /*, last_win_error_str()*/);
                    return nullptr;
                }
                glad_initialized = true;
            }
        }

        const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        const char* glsl_version = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
        ::logger.info("Vendor:   {}", fmt::styled_info(vendor));
        ::logger.info("Renderer: {}", fmt::styled_info(renderer));
        ::logger.info("GL Version: {}", fmt::styled_info(version));
        ::logger.info("GLSL Version: {}", fmt::styled_info(glsl_version));

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

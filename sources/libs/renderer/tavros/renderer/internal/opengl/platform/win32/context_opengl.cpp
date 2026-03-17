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

// WGL_ARB_create_context constants - defined manually to avoid depending on wglext.h.
// See: https://registry.khronos.org/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
#define WGL_CONTEXT_MAJOR_VERSION_ARB    0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB    0x2092
#define WGL_CONTEXT_FLAGS_ARB            0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB     0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_DEBUG_BIT_ARB        0x0001
#define WGL_DRAW_TO_WINDOW_ARB           0x2001
#define WGL_SUPPORT_OPENGL_ARB           0x2010
#define WGL_DOUBLE_BUFFER_ARB            0x2011
#define WGL_PIXEL_TYPE_ARB               0x2013
#define WGL_TYPE_RGBA_ARB                0x202B
#define WGL_COLOR_BITS_ARB               0x2014
#define WGL_DEPTH_BITS_ARB               0x2022
#define WGL_STENCIL_BITS_ARB             0x2023

namespace
{
    tavros::core::logger logger("gl_context");

    // -------------------------------------------------------------------------
    // WGL function pointer types and globals
    //
    // These are loaded once from opengl32.dll and via wglGetProcAddress.
    // They are intentionally file-scoped - nothing outside this TU needs them.
    // -------------------------------------------------------------------------

    // Base WGL functions - available directly in opengl32.dll
    using fn_wglCreateContext_t = HGLRC(WINAPI*)(HDC);
    using fn_wglDeleteContext_t = BOOL(WINAPI*)(HGLRC);
    using fn_wglMakeCurrent_t = BOOL(WINAPI*)(HDC, HGLRC);
    using fn_wglGetCurrentContext_t = HGLRC(WINAPI*)(void);
    using fn_wglGetProcAddress_t = void*(WINAPI*) (const char*);

    fn_wglCreateContext_t     g_wglCreateContext = nullptr;
    fn_wglDeleteContext_t     g_wglDeleteContext = nullptr;
    fn_wglMakeCurrent_t       g_wglMakeCurrent = nullptr;
    fn_wglGetCurrentContext_t g_wglGetCurrentContext = nullptr;
    fn_wglGetProcAddress_t    g_wglGetProcAddress = nullptr;

    // Extended WGL functions - require an active context to be loaded
    using fn_wglCreateContextAttribsARB_t = HGLRC(WINAPI*)(HDC, HGLRC, const int*);
    using fn_wglChoosePixelFormatARB_t = BOOL(WINAPI*)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
    using fn_wglSwapIntervalEXT_t = BOOL(WINAPI*)(int);

    fn_wglCreateContextAttribsARB_t g_wglCreateContextAttribsARB = nullptr;
    fn_wglChoosePixelFormatARB_t    g_wglChoosePixelFormatARB = nullptr;
    fn_wglSwapIntervalEXT_t         g_wglSwapIntervalEXT = nullptr;

    HMODULE g_gl_lib = nullptr;

    // -------------------------------------------------------------------------
    // load_wgl_base
    //
    // Loads the fundamental WGL entry points directly from opengl32.dll.
    // These functions exist even without a valid GL context and must be
    // loaded before any context or extension work can begin.
    // -------------------------------------------------------------------------
    bool load_wgl_base() noexcept
    {
        if (!g_gl_lib) {
            g_gl_lib = LoadLibraryA("opengl32.dll");
            if (!g_gl_lib) {
                logger.error("Failed to load opengl32.dll.");
                return false;
            }
        }

        g_wglCreateContext = reinterpret_cast<fn_wglCreateContext_t>(GetProcAddress(g_gl_lib, "wglCreateContext"));
        g_wglDeleteContext = reinterpret_cast<fn_wglDeleteContext_t>(GetProcAddress(g_gl_lib, "wglDeleteContext"));
        g_wglMakeCurrent = reinterpret_cast<fn_wglMakeCurrent_t>(GetProcAddress(g_gl_lib, "wglMakeCurrent"));
        g_wglGetCurrentContext = reinterpret_cast<fn_wglGetCurrentContext_t>(GetProcAddress(g_gl_lib, "wglGetCurrentContext"));
        g_wglGetProcAddress = reinterpret_cast<fn_wglGetProcAddress_t>(GetProcAddress(g_gl_lib, "wglGetProcAddress"));

        if (!g_wglCreateContext || !g_wglDeleteContext || !g_wglMakeCurrent || !g_wglGetCurrentContext || !g_wglGetProcAddress) {
            logger.error("Failed to load one or more base WGL functions.");
            return false;
        }

        return true;
    }

    // -------------------------------------------------------------------------
    // load_wgl_extensions
    //
    // Loads extended WGL functions via wglGetProcAddress.
    // Must be called while a GL context is current.
    // -------------------------------------------------------------------------
    bool load_wgl_extensions() noexcept
    {
        g_wglCreateContextAttribsARB = reinterpret_cast<fn_wglCreateContextAttribsARB_t>(g_wglGetProcAddress("wglCreateContextAttribsARB"));
        g_wglChoosePixelFormatARB = reinterpret_cast<fn_wglChoosePixelFormatARB_t>(g_wglGetProcAddress("wglChoosePixelFormatARB"));
        g_wglSwapIntervalEXT = reinterpret_cast<fn_wglSwapIntervalEXT_t>(g_wglGetProcAddress("wglSwapIntervalEXT"));

        if (!g_wglCreateContextAttribsARB) {
            logger.error("wglCreateContextAttribsARB not available. Driver may not support OpenGL 4.6 Core Profile.");
            return false;
        }

        if (!g_wglChoosePixelFormatARB) {
            logger.error("wglChoosePixelFormatARB not available.");
            return false;
        }

        if (!g_wglSwapIntervalEXT) {
            logger.warning("wglSwapIntervalEXT not available. VSync control will not work.");
            // Non-fatal - proceed without vsync control
        }

        return true;
    }

    // -------------------------------------------------------------------------
    // make_bootstrap_context
    //
    // WGL extension functions (wglCreateContextAttribsARB, wglChoosePixelFormatARB,
    // etc.) can only be retrieved via wglGetProcAddress, which itself requires an
    // active GL context. This creates a chicken-and-egg problem: we need extensions
    // to create a proper context, but we need a context to load the extensions.
    //
    // The standard solution is a "bootstrap" (dummy) context:
    //   1. Create a throwaway window with a legacy pixel format.
    //   2. Create a legacy GL 1.x context on it.
    //   3. Activate it just long enough to call wglGetProcAddress.
    //   4. Destroy everything - the real context will be created on the actual window.
    //
    // Fallback: if window creation fails (e.g. in headless/service environments),
    // we attempt to create a bootstrap context directly on the target HDC instead.
    // -------------------------------------------------------------------------
    bool make_bootstrap_context(HDC fallback_dc) noexcept
    {
        // --- Attempt 1: dedicated dummy window ---

        WNDCLASSA wc = {};
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = DefWindowProcA;
        wc.hInstance = GetModuleHandleA(nullptr);
        wc.lpszClassName = "tavros_wgl_bootstrap";
        RegisterClassA(&wc); // Ignore ERROR_CLASS_ALREADY_EXISTS

        HWND dummy_wnd = CreateWindowExA(
            0, "tavros_wgl_bootstrap", "",
            WS_OVERLAPPEDWINDOW,
            0, 0, 1, 1,
            nullptr, nullptr, GetModuleHandleA(nullptr), nullptr
        );

        HDC  bootstrap_dc = nullptr;
        HWND bootstrap_wnd = nullptr;
        bool owns_window = false;

        if (dummy_wnd) {
            bootstrap_wnd = dummy_wnd;
            bootstrap_dc = GetDC(dummy_wnd);
            owns_window = true;
        } else {
            // --- Fallback: use the real window's DC ---
            logger.warning("Bootstrap window creation failed (error {}). Falling back to target DC.", GetLastError());
            bootstrap_dc = fallback_dc;
        }

        if (!bootstrap_dc) {
            logger.error("Failed to obtain a DC for bootstrap context.");
            if (owns_window) {
                DestroyWindow(bootstrap_wnd);
            }
            return false;
        }

        // Set a minimal legacy pixel format - required before creating any context
        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.iLayerType = PFD_MAIN_PLANE;

        const int fmt = ChoosePixelFormat(bootstrap_dc, &pfd);
        if (!fmt || !SetPixelFormat(bootstrap_dc, fmt, &pfd)) {
            logger.error("Failed to set pixel format for bootstrap context (error {}).", GetLastError());
            if (owns_window) {
                ReleaseDC(bootstrap_wnd, bootstrap_dc);
                DestroyWindow(bootstrap_wnd);
            }
            return false;
        }

        HGLRC bootstrap_ctx = g_wglCreateContext(bootstrap_dc);
        if (!bootstrap_ctx) {
            logger.error("Failed to create bootstrap GL context (error {}).", GetLastError());
            if (owns_window) {
                ReleaseDC(bootstrap_wnd, bootstrap_dc);
                DestroyWindow(bootstrap_wnd);
            }
            return false;
        }

        if (!g_wglMakeCurrent(bootstrap_dc, bootstrap_ctx)) {
            logger.error("Failed to activate bootstrap GL context (error {}).", GetLastError());
            g_wglDeleteContext(bootstrap_ctx);
            if (owns_window) {
                ReleaseDC(bootstrap_wnd, bootstrap_dc);
                DestroyWindow(bootstrap_wnd);
            }
            return false;
        }

        const bool ok = load_wgl_extensions();

        g_wglMakeCurrent(nullptr, nullptr);
        g_wglDeleteContext(bootstrap_ctx);
        if (owns_window) {
            ReleaseDC(bootstrap_wnd, bootstrap_dc);
            DestroyWindow(bootstrap_wnd);
        }

        return ok;
    }

    // -------------------------------------------------------------------------
    // glad_loader
    //
    // Custom loader passed to gladLoadGLLoader. Tries wglGetProcAddress first
    // (for extension functions), then falls back to GetProcAddress on opengl32.dll
    // (for core functions like glGetString that are exported directly).
    // -------------------------------------------------------------------------
    void* glad_loader(const char* name) noexcept
    {
        void* p = reinterpret_cast<void*>(g_wglGetProcAddress(name));
        if (!p || p == reinterpret_cast<void*>(1) || p == reinterpret_cast<void*>(2) || p == reinterpret_cast<void*>(3) || p == reinterpret_cast<void*>(-1)) {
            p = reinterpret_cast<void*>(GetProcAddress(g_gl_lib, name));
        }
        return p;
    }

} // namespace

namespace tavros::renderer::rhi
{

    core::unique_ptr<context_opengl> context_opengl::create(const frame_composer_create_info& info, void* native_handle)
    {
        if (info.width == 0 || info.height == 0) {
            ::logger.error("Swapchain width and height must be greater than zero.");
            return nullptr;
        }

        if (info.buffer_count != 2 && info.buffer_count != 3) {
            ::logger.error("Swapchain buffer count must be 2 or 3.");
            return nullptr;
        }

        const auto color_info = to_gl_wnd_fb_info(info.color_attachment_format);
        if (!color_info.supported || color_info.color_bits == 0) {
            ::logger.error("Swapchain color format {} is not supported.", info.color_attachment_format);
            return nullptr;
        }

        const auto depth_info = to_gl_wnd_fb_info(info.depth_stencil_attachment_format);
        const bool depth_ok = depth_info.supported && (depth_info.depth_bits > 0 || depth_info.stencil_bits > 0);
        if (!depth_ok && info.depth_stencil_attachment_format != rhi::pixel_format::none) {
            ::logger.error("Swapchain depth/stencil format {} is not supported.", info.depth_stencil_attachment_format);
            return nullptr;
        }

        TAV_ASSERT(native_handle);
        HWND hWnd = static_cast<HWND>(native_handle);

        static bool       s_glad_initialized = false;
        static bool       s_ext_loaded = false;
        static std::mutex s_mtx;
        std::lock_guard   lock(s_mtx);

        if (!load_wgl_base()) {
            return nullptr;
        }

        HDC hDC = GetDC(hWnd);
        if (!hDC) {
            ::logger.error("GetDC failed (error {}).", GetLastError());
            return nullptr;
        }
        auto dc_guard = core::make_scoped_owner(hDC, [hWnd](HDC dc) { ReleaseDC(hWnd, dc); });

        if (!s_ext_loaded) {
            if (!make_bootstrap_context(hDC)) {
                return nullptr;
            }
            s_ext_loaded = true;
        }

        // Choose a hardware-accelerated pixel format matching the requested attachments
        const int pf_attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB, static_cast<int>(color_info.color_bits),
            WGL_DEPTH_BITS_ARB, static_cast<int>(depth_info.depth_bits),
            WGL_STENCIL_BITS_ARB, static_cast<int>(depth_info.stencil_bits),
            0
        };

        int  pixel_format = 0;
        UINT num_formats = 0;
        if (!g_wglChoosePixelFormatARB(hDC, pf_attribs, nullptr, 1, &pixel_format, &num_formats) || num_formats == 0) {
            ::logger.error("wglChoosePixelFormatARB failed - no matching pixel format.");
            return nullptr;
        }

        // Log what the driver actually selected
        PIXELFORMATDESCRIPTOR chosen_pfd = {};
        DescribePixelFormat(hDC, pixel_format, sizeof(chosen_pfd), &chosen_pfd);
        ::logger.info("Pixel format: ColorBits={}, DepthBits={}, StencilBits={}", fmt::styled_param(chosen_pfd.cColorBits), fmt::styled_param(chosen_pfd.cDepthBits), fmt::styled_param(chosen_pfd.cStencilBits));

        if (chosen_pfd.dwFlags & PFD_GENERIC_FORMAT) {
            ::logger.error("Software (GDI) renderer selected - no hardware OpenGL driver available.");
            return nullptr;
        }

        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        if (!SetPixelFormat(hDC, pixel_format, &pfd)) {
            ::logger.error("SetPixelFormat failed (error {}).", GetLastError());
            return nullptr;
        }

        // Create an OpenGL 4.6 Core Profile context
        const int ctx_attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 6,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifndef NDEBUG
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
            0
        };

        HGLRC hGLRC = g_wglCreateContextAttribsARB(hDC, nullptr, ctx_attribs);
        if (!hGLRC) {
            ::logger.error("wglCreateContextAttribsARB failed - OpenGL 4.6 Core Profile not supported.");
            return nullptr;
        }

        if (!g_wglMakeCurrent(hDC, hGLRC)) {
            g_wglDeleteContext(hGLRC);
            ::logger.error("Failed to activate GL context (error {}).", GetLastError());
            return nullptr;
        }

        // Load all GL function pointers via GLAD (done once per process)
        if (!s_glad_initialized) {
            if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glad_loader))) {
                g_wglMakeCurrent(nullptr, nullptr);
                g_wglDeleteContext(hGLRC);
                ::logger.error("gladLoadGLLoader failed.");
                return nullptr;
            }
            s_glad_initialized = true;
        }

        ::logger.info("Vendor:       {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        ::logger.info("Renderer:     {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        ::logger.info("GL Version:   {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
        ::logger.info("GLSL Version: {}", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

        if (g_wglSwapIntervalEXT && info.swap_interval != -1) {
            g_wglSwapIntervalEXT(static_cast<int>(info.swap_interval));
        }

        return core::make_unique<context_opengl_win32>(hWnd, dc_guard.release(), hGLRC);
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
            if (!g_wglDeleteContext(m_hGLRC)) {
                ::logger.error("Delete OpenGL context failed");
            }
            m_hGLRC = nullptr;
        }

        if (m_hDC) {
            if (!ReleaseDC(m_hWnd, m_hDC)) {
                ::logger.error("ReleaseDC failed");
            }
            m_hDC = nullptr;
        }

        m_hWnd = nullptr;
    }

    void context_opengl_win32::make_current()
    {
        if (!g_wglMakeCurrent(m_hDC, m_hGLRC)) {
            ::logger.error("OpenGL activate context failed");
        }
    }

    void context_opengl_win32::make_inactive()
    {
        if (!g_wglMakeCurrent(nullptr, nullptr)) {
            ::logger.error("OpenGL deactivate context failed");
        }
    }

    void context_opengl_win32::swap_buffers()
    {
        if (!SwapBuffers(m_hDC)) {
            ::logger.error("OpenGL swap buffers failed");
        }
    }

    bool context_opengl_win32::is_current() const noexcept
    {
        return g_wglGetCurrentContext() == m_hGLRC;
    }

} // namespace tavros::renderer::rhi

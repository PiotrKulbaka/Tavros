#include <tavros/renderer/internal/opengl/platform/win32/swapchain_opengl.hpp>

#include <tavros/renderer/internal/opengl/swapchain_opengl.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/scoped_owner.hpp>

#include <WinUser.h>
#include <windef.h>
#include <wingdi.h>

namespace
{
    tavros::core::logger logger("swapchain_opengl");

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

namespace tavros::renderer
{

    core::shared_ptr<swapchain> create_swapchain_opengl(graphics_device_opengl* device, const swapchain_desc& desc, void* native_handle)
    {
        // Validate the swapchain desc
        // Validate width and height
        if (desc.width == 0 || desc.height == 0) {
            ::logger.error("Swapchain width and height must be greater than zero.");
            return nullptr;
        }

        // Validate buffer count
        if (desc.buffer_count == 0) {
            ::logger.error("Swapchain buffer count must be greater than zero.");
            return nullptr;
        }

        if (desc.buffer_count != 2 && desc.buffer_count != 3) {
            ::logger.error("Swapchain buffer count must be 2 or 3.");
            return nullptr;
        }

        // Validate color attachment
        if (desc.color_attachment_format != pixel_format::rgba8un) {
            ::logger.error("Swapchain color attachment format must be rgba8un.");
            return nullptr;
        }

        // Validate depth/stencil attachment
        if (desc.depth_stencil_attachment_format != pixel_format::depth24_stencil8 && desc.depth_stencil_attachment_format != pixel_format::none) {
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
        pfd.cDepthBits = desc.depth_stencil_attachment_format == pixel_format::depth24_stencil8 ? 24 : 0;
        pfd.cStencilBits = desc.depth_stencil_attachment_format == pixel_format::depth24_stencil8 ? 8 : 0;
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

        return core::make_shared<swapchain_opengl>(device, desc, hWnd, dc_owner.release(), hGLRC);
    }


    swapchain_opengl::swapchain_opengl(graphics_device_opengl* device, const swapchain_desc& desc, HWND hWnd, HDC hDC, HGLRC hGLRC)
        : m_device(device)
        , m_desc(desc)
        , m_hWnd(hWnd)
        , m_hDC(hDC)
        , m_hGLRC(hGLRC)
        , m_width(desc.width)
        , m_height(desc.height)
    {
        TAV_ASSERT(hWnd);
        TAV_ASSERT(hDC);
        TAV_ASSERT(hGLRC);

        framebuffer_desc fb_screen;
        fb_screen.width = m_width;
        fb_screen.height = m_height;
        fb_screen.color_attachment_formats.push_back(desc.color_attachment_format);
        fb_screen.depth_stencil_attachment_format = desc.depth_stencil_attachment_format;

        // Create screen framebuffer
        m_framebuffer = {m_device->get_resources()->framebuffers.insert({fb_screen, 0, true})};

        ::logger.debug("Default framebuffer with id %u for swapchain", m_framebuffer.id);

        make_current();

        static bool glad_initialized = false;
        if (!glad_initialized) {
            glad_initialized = true;
            if (!gladLoadGL()) {
                throw std::runtime_error("Failed to initialize OpenGL context via GLAD");
            }
        }
    }

    swapchain_opengl::~swapchain_opengl()
    {
        // Don't destroy if has no framebuffers (because now destructor of graphics_device is called)
        if (m_device->get_resources()->framebuffers.size() != 0) {
            if (auto* desc = m_device->get_resources()->framebuffers.try_get(m_framebuffer.id)) {
                m_device->get_resources()->framebuffers.remove(m_framebuffer.id);
            } else {
                ::logger.error("Can't destroy default framebuffer with id %u because it doesn't exist", m_framebuffer.id);
            }
        }

        // Destroy OpenGL context
        if (m_hGLRC) {
            if (!_wglDeleteContext(m_hGLRC)) {
                ::logger.error("Detete OpenGL context failed" /*, last_win_error_str()*/);
            }
            m_hGLRC = nullptr;
        }

        // Destroy DC
        if (m_hDC) {
            if (!ReleaseDC(m_hWnd, m_hDC)) {
                ::logger.error("ReleaseDC failed.");
            }
            m_hDC = nullptr;
        }
        m_hWnd = nullptr;
    }

    uint32 swapchain_opengl::acquire_next_backbuffer_index() noexcept
    {
        return 0;
    }

    framebuffer_handle swapchain_opengl::get_framebuffer(uint32 backbuffer_index)
    {
        TAV_ASSERT(backbuffer_index == 0);

        return m_framebuffer;
    }

    void swapchain_opengl::present(uint32 backbuffer_index)
    {
        TAV_ASSERT(backbuffer_index == 0);

        if (!SwapBuffers(m_hDC)) {
            ::logger.error("OpenGL swap buffers failed" /*, last_win_error_str()*/);
        }
    }

    void swapchain_opengl::resize(uint32 width, uint32 height)
    {
        TAV_ASSERT(width > 0 && height > 0);

        m_width = width;
        m_height = height;

        if (auto* desc = m_device->get_resources()->framebuffers.try_get(m_framebuffer.id)) {
            desc->desc.width = m_width;
            desc->desc.height = m_height;
        } else {
            ::logger.error("Can't find default framebuffer with id %u", m_framebuffer.id);
        }
    }

    uint32 swapchain_opengl::width() const noexcept
    {
        return m_width;
    }

    uint32 swapchain_opengl::height() const noexcept
    {
        return m_height;
    }

    void swapchain_opengl::make_current()
    {
        if (!_wglMakeCurrent(m_hDC, m_hGLRC)) {
            ::logger.error("OpenGL context activation failed" /*, last_win_error_str()*/);
        }
    }

    void swapchain_opengl::make_inactive()
    {
        if (!_wglMakeCurrent(nullptr, nullptr)) {
            ::logger.error("OpenGL deactivate context failed" /*, last_win_error_str()*/);
        }
    }

} // namespace tavros::renderer

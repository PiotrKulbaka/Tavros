#include <tavros/system/platform/win32/application.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/system/platform/win32/platform_window.hpp>

#include <Windows.h>
#include <shellscalingapi.h>

namespace
{

    static tavros::core::logger logger("application");

    void enable_high_dpi_awareness()
    {
        // Windows 10 Anniversary Update or later
        if (auto hUser32 = GetModuleHandleW(L"user32.dll")) {
            if (auto setDpiAwarenessContext =
                    reinterpret_cast<BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT)>(
                        GetProcAddress(hUser32, "SetProcessDpiAwarenessContext")
                    )) {
                if (setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
                    return;
                }
            }
        }

        // Windows 8.1+
        if (auto hShcore = GetModuleHandleW(L"Shcore.dll")) {
            if (auto setProcessDpiAwareness =
                    reinterpret_cast<HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS)>(
                        GetProcAddress(hShcore, "SetProcessDpiAwareness")
                    )) {
                setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
                return;
            }
        }

        // Windows Vista / 7 / 8
        if (auto hUser32 = GetModuleHandleW(L"user32.dll")) {
            if (auto setProcessDPIAware =
                    reinterpret_cast<BOOL(WINAPI*)()>(
                        GetProcAddress(hUser32, "SetProcessDPIAware")
                    )) {
                setProcessDPIAware();
                return;
            }
        }
    }

    static atomic_size_t s_windows_count = 0;

} // namespace

namespace tavros::system
{
    tavros::core::unique_ptr<application> application::create()
    {
        return tavros::core::make_unique<tavros::system::win32::application>();
    }
} // namespace tavros::system

namespace tavros::system::win32
{

    void increase_windows_count()
    {
        ++s_windows_count;
    }

    void decrease_windows_count()
    {
        --s_windows_count;
    }

    application::application()
        : m_is_running(false)
        , m_exit_code(0)
    {
        enable_high_dpi_awareness();
    }

    application::~application()
    {
    }

    int application::run()
    {
        m_is_running = true;

        poll_events();
        while (m_is_running && s_windows_count.load(std::memory_order_relaxed) > 0) {
            wait_events();
            poll_events();
        }
        return m_exit_code;
    }

    void application::exit(int exit_code)
    {
        m_exit_code = exit_code;
        m_is_running = false;
    }

    void application::poll_events()
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                exit(0);
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void application::wait_events()
    {
        MsgWaitForMultipleObjects(0, nullptr, FALSE, INFINITE, QS_ALLINPUT);
    }

    tavros::math::isize2 application::desktop_size()
    {
        RECT       desktopRect;
        const HWND hDesktop = GetDesktopWindow();
        if (GetWindowRect(hDesktop, &desktopRect)) {
            int32 width = desktopRect.right - desktopRect.left;
            int32 height = desktopRect.bottom - desktopRect.top;
            return {width, height};
        }
        return {0, 0};
    }

} // namespace tavros::system::win32
